#include "downloadmanager.h"
#include <QtConcurrent>
#include "player/playlistitem.h"
#include "providers/showprovider.h"
#include "utils/errorhandler.h"
#include <player/serverlist.h>
#include <memory>

DownloadManager::DownloadManager(QObject *parent): QAbstractListModel(parent) {
    N_m3u8DLPath = QDir::cleanPath (QCoreApplication::applicationDirPath() + QDir::separator() + "N_m3u8DL-RE.exe");
    m_ffmpegPath = QDir::cleanPath (QCoreApplication::applicationDirPath() + QDir::separator() + "ffmpeg.exe");
    m_isWorking = QFileInfo::exists(N_m3u8DLPath) && QFileInfo::exists(m_ffmpegPath);

    //m_workDir = QDir::cleanPath (QCoreApplication::applicationDirPath() + QDir::separator() + "Downloads");

    if (!m_isWorking) return;

    m_workDir = QDir::cleanPath("D:\\TV\\Downloads");
    constexpr int threadCount = 4 ;
    //        pool.setMaxThreadCount(threadCount);

    for (int i = 0; i < threadCount; ++i) {
        auto watcher = new QFutureWatcher<bool>();
        watchers.push_back (watcher);
        QObject::connect (watcher, &QFutureWatcher<bool>::finished, this, [watcher, this](){
            // Set task success
            if (!watcher->future().isValid()) {
                qDebug() << "Log (Downloader) :" << watcherTaskTracker[watcher]->displayName << "cancelled successfully";
            } else {
                try {
                    auto res = watcher->future().result();
                } catch (...) {
                    qDebug() << "Log (Downloader) :" << watcherTaskTracker[watcher]->displayName << "task failed";
                    ErrorHandler::instance().show (QString("Failed to download %1").arg(watcherTaskTracker[watcher]->displayName), "Download Error");
                }
            }
            removeTask(watcherTaskTracker[watcher]);
            watchTask (watcher);
            emit layoutChanged();
        });

        QObject::connect (watcher, &QFutureWatcher<bool>::progressValueChanged, this, [watcher, this](){
            Q_ASSERT(watcherTaskTracker[watcher]);
            watcherTaskTracker[watcher]->progressValue = watcher->progressValue();
            int i = tasks.indexOf (watcherTaskTracker[watcher]);
            emit dataChanged(index(i, 0),index(i, 0));
        });
        QObject::connect (watcher, &QFutureWatcher<bool>::progressTextChanged, this, [watcher, this](){
            Q_ASSERT(watcherTaskTracker[watcher]);
            watcherTaskTracker[watcher]->progressText = watcher->progressText();
            int i = tasks.indexOf (watcherTaskTracker[watcher]);
            emit dataChanged(index(i, 0),index(i, 0));
        });
    }
}

void DownloadManager::removeTask(std::shared_ptr<DownloadTask> &task) {
    QMutexLocker locker(&mutex);
    tasks.removeOne (task);
    if (!task->watcher) return;
    // task has started, not in task queue
    Q_ASSERT(task == watcherTaskTracker[task->watcher]);
    if (task->watcher->isRunning()){
        qDebug() << "Log (Downloader) : Attempting to cancel the ongoing task" << task->displayName;
        task->watcher->cancel();
        task->watcher->waitForFinished();
        return; // this function is called again from the canceled signal slot
    }
    watcherTaskTracker[task->watcher] = nullptr;
    qDebug() << "Log (Downloader) : Removed task" << task->displayName;

    emit layoutChanged();

}

void DownloadManager::watchTask(QFutureWatcher<bool> *watcher)
{
    QMutexLocker locker(&mutex);
    if (tasksQueue.isEmpty())
        return;
    auto task = tasksQueue.dequeue ();
    if (task.expired()) // task could have been cancelled while queued
        return;

    auto taskPtr = task.lock();
    taskPtr->watcher = watcher;

    watcherTaskTracker[watcher] = taskPtr;
    // QStringList command {taskPtr->link,"--workDir", taskPtr->folder,"--saveName", taskPtr->videoName, "--enableDelAfterDone", "--disableDateInfo"};
    QStringList command {taskPtr->link,
                        "--save-dir", taskPtr->folder,
                        "--save-name", taskPtr->videoName,
                        "--ffmpeg-binary-path",  m_ffmpegPath,
                        "--del-after-done", "--no-date-info",
                        "--auto-select"};
    if (!taskPtr->headers.isEmpty()) {
        for (auto it = taskPtr->headers.begin(); it != taskPtr->headers.end(); ++it) {
            command.append("-H");
            command.append(it.key() + ": " + it.value());
        }
    }
    qDebug() << command;
    watcher->setFuture (QtConcurrent::run (&DownloadManager::executeCommand, this, command));
}

void DownloadManager::enqueue(std::shared_ptr<DownloadTask> &task) {
    QMutexLocker locker(&mutex);
    std::weak_ptr<DownloadTask> weakPtr = task;
    tasksQueue.enqueue(weakPtr);
}

void DownloadManager::downloadLink(const QString &name, const QString &link) {
    if (!m_isWorking)
        return;

    auto task = std::make_shared<DownloadTask>(name, m_workDir, link, QHash<QString, QString>(), name , link);
    auto client = Client(nullptr);
    if (!client.isOk(link)) {
        ErrorHandler::instance().show ("Invalid URL", "Download Error");
        return;
    }
    enqueue(task);
    startTasks();
    emit layoutChanged();
}

void DownloadManager::executeCommand(QPromise<bool> &promise, const QStringList &command) {
    if (!m_isWorking) return;

    promise.setProgressRange (0, 100);
    QProcess process;
    process.setProgram (N_m3u8DLPath);
    process.setArguments (command);
    static QRegularExpression percentRegex = QRegularExpression(R"((\d+\.\d+)%)");

    process.start();
    float percent = 0;




    while (process.state() == QProcess::Running
           && process.waitForReadyRead()
           && !promise.isCanceled())
    {
        auto line = process.readAll().trimmed();
        qDebug() << line;
        QRegularExpressionMatch match = percentRegex.match(line);
        if (match.hasMatch()) {
            percent = match.captured(1).toFloat();
            // promise.setProgressValue (percent);
        } else if (line.contains("404 (Not Found)")) {
            promise.addResult (false); // TODO add reason
            break;
        }
        promise.setProgressValueAndText(percent+0.001, line);
    }
    if (!promise.isCanceled()) {
        process.waitForFinished(-1);
        promise.addResult (true);
    } else {
        promise.addResult (false);
        process.kill();
    }
}




void DownloadManager::downloadShow(ShowData &show, int startIndex, int count) {
    auto playlist = show.getPlaylist();
    if (!playlist || count < 1) return;

    playlist->use(); // Prevents the playlist from being deleted whilst using it
    QString showName = QString(show.title).replace(":",".").replace(folderNameCleanerRegex, "_");   //todo check replace
    auto provider = show.getProvider();
    int endIndex = startIndex + count;
    if (endIndex > playlist->size()) endIndex = playlist->size();
    qDebug() << "Log (Downloader)" << showName << "from index" << startIndex << "to" << endIndex - 1;

    QFuture<void> future = QtConcurrent::run([this, showName, playlist, provider , startIndex, endIndex](){
        try {
            auto client = Client(nullptr);
            for (int i = startIndex; i < endIndex; ++i) {
                const PlaylistItem* episode = playlist->at(i);
                QString episodeName = episode->getFullName().trimmed().replace("\n", ". ");
                QString workDir = QDir::cleanPath (m_workDir + QDir::separator() + showName);
                QString displayName = showName + " : " + episodeName;
                QString path = QDir::cleanPath (workDir + QDir::separator() + episodeName + ".mp4");
                auto task = std::make_shared<DownloadTask>(episodeName, workDir, QString(), QHash<QString, QString>(), displayName, path);
                qDebug() << "Log (Downloader) : Appending new download task for" << episodeName;

                tasks.push_back(std::move(task));
                emit layoutChanged();

                QList<VideoServer> servers = provider->loadServers(&client, episode);
                PlayInfo playInfo = ServerList::autoSelectServer(&client, servers, provider);
                if (playInfo.sources.isEmpty()) {
                    qDebug() << "Downloader no links found for" << episodeName;
                    return;
                }
                auto videoToDownload = playInfo.sources.first();
                task->link = videoToDownload.videoUrl.toString();
                task->headers = videoToDownload.getHeaders();
                qDebug() << "Log (Downloader) : Starting download for" << episodeName;
            }
            startTasks();
            playlist->disuse();

        }catch(const QException &ex) {
            ErrorHandler::instance().show (ex.what(), "Download Error");
        }

    });

}

void DownloadManager::cancelTask(int index) {
    if (index >= 0 && index < tasks.size()) {
        removeTask(tasks[index]);
        emit layoutChanged();
    }
}

void DownloadManager::cancelAllTasks() {
    QMutexLocker locker(&mutex);
    for (auto *watcher : watchers) {
        if (!watcherTaskTracker[watcher])
            continue;
        watcherTaskTracker[watcher] = nullptr;
        watcher->cancel();
        watcher->waitForFinished();
    }

    tasks.clear();
    tasksQueue.clear();
}

void DownloadManager::startTasks() {
    QMutexLocker locker(&mutex);
    for (auto* watcher:watchers) {
        if (tasksQueue.isEmpty()) break;
        else if (!watcherTaskTracker[watcher]) watchTask (watcher); //if watcher not working on a task
    }
}

bool DownloadManager::setWorkDir(const QString &path) {
    const QFileInfo outputDir(path);
    if ((!outputDir.exists()) || (!outputDir.isDir()) || (!outputDir.isWritable())) {
        qWarning() << "Log (Downloader) : Output directory either doesn't exist or isn't a directory or writeable"
                   << outputDir.absoluteFilePath();
        return false;
    }
    m_workDir = path;

    emit workDirChanged();
    return true;
}

QVariant DownloadManager::data(const QModelIndex &index, int role) const {
    if (!index.isValid())
        return QVariant();

    auto task = tasks.at (index.row());

    switch (role){
    case NameRole:
        return task->displayName;
        break;
    case PathRole:
        return task->path;
        break;
    case ProgressValueRole:
        return task->progressValue;
        break;
    case ProgressTextRole:
        return task->progressText;
        break;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> DownloadManager::roleNames() const{
    QHash<int, QByteArray> names;
    names[NameRole] = "downloadName";
    names[PathRole] = "downloadPath";
    names[ProgressValueRole] = "progressValue";
    names[ProgressTextRole] = "progressText";
    return names;
}
