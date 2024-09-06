#include "downloadmanager.h"
#include <QtConcurrent>
#include "player/playlistitem.h"
#include "providers/showprovider.h"
#include "utils/errorhandler.h"
#include "player/serverlistmodel.h"
#include <memory>

DownloadManager::DownloadManager(QObject *parent): QAbstractListModel(parent) {


    m_workDir = QDir::cleanPath("D:\\TV\\Downloads");
    constexpr int maxConcurrentTask = 8 ;
    for (int i = 0; i < maxConcurrentTask; ++i) {
        auto watcher = new QFutureWatcher<void>();
        watchers.push_back(watcher);
        QObject::connect (watcher, &QFutureWatcher<void>::finished, this, [watcher, this](){
            // Set task success
            auto task = watcherTaskTracker[watcher];
            if (!watcher->future().isValid()) {
                qDebug() << "Log (Downloader) :" << task->displayName << "cancelled successfully";
            } else {
                try {
                    watcher->future().waitForFinished();
                } catch (...) {
                    qDebug() << "Log (Downloader) :" << task->displayName << "task failed";
                    ErrorHandler::instance().show (QString("Failed to download %1").arg(task->displayName), "Download Error");
                }
            }
            removeTask(task);
            watchTask(watcher);
        });


    }
}


void DownloadManager::downloadLink(const QString &name, const QString &link) {
    if (!DownloadTask::checkDependencies()) return;
    beginInsertRows(QModelIndex(), tasks.size(), tasks.size());
    auto cleanedName = cleanFolderName(name);
    QString path = QDir::cleanPath(m_workDir + QDir::separator() + cleanedName + ".mp4");
    if (QFile::exists(path)) {
        qDebug() << "Log (Downloader) : File already exists" << path;
        return;
    }
    tasks.push_back(std::move(
        std::make_shared<DownloadTask>(name, m_workDir, link, cleanedName)
        ));
    endInsertRows();
    auto future = QtConcurrent::run([&](){
        auto client = Client(nullptr);
        if (!client.isOk(link)) {
            ErrorHandler::instance().show ("Invalid URL", "Download Error");
            removeTask(tasks.back());
            return;
        }
        std::weak_ptr<DownloadTask> weakPtr = tasks.back();
        tasksQueue.enqueue(weakPtr);
        startTasks();
    });

}


void DownloadManager::downloadShow(ShowData &show, int startIndex, int count) {
    if (!DownloadTask::checkDependencies()) return;

    auto playlist = show.getPlaylist();
    if (!playlist || count < 1) return;
    playlist->use(); // Prevents the playlist from being deleted whilst using it
    QString showName = cleanFolderName(show.title);   //todo check replace
    auto provider = show.getProvider();
    int endIndex = startIndex + count;
    if (endIndex > playlist->size()) endIndex = playlist->size();
    qDebug() << "Log (Downloader)" << showName << "from index" << startIndex << "to" << endIndex - 1;

    QFuture<void> future = QtConcurrent::run([this, showName, playlist, provider , startIndex, endIndex](){
        auto client = Client(nullptr);
        QString workDir = QDir::cleanPath(m_workDir + QDir::separator() + showName);
        for (int i = startIndex; i < endIndex; ++i) {
            const PlaylistItem* episode = playlist->at(i);
            QString episodeName = episode->getFullName().trimmed().replace("\n", ". ");
            QString displayName = showName + " : " + episodeName;
            QString path = QDir::cleanPath (workDir + QDir::separator() + episodeName + ".mp4");
            // Check if path exists
            if (QFile::exists(path)) {
                qDebug() << "Log (Downloader) : File already exists" << path;
                continue;
            }

            qDebug() << "Log (Downloader) : Appending new download task for" << episodeName;
            std::weak_ptr<DownloadTask> taskWeakPtr;
            {
                QMutexLocker locker(&mutex);
                beginInsertRows(QModelIndex(), tasks.size(), tasks.size());
                tasks.push_back(std::move(
                    std::make_shared<DownloadTask>(episodeName, workDir, QString(), displayName)
                    ));
                endInsertRows();
                taskWeakPtr = tasks.back();
                tasks.back()->setProgressText("Extracting source...");
            }

            QList<VideoServer> servers = provider->loadServers(&client, episode);
            PlayInfo playInfo = ServerListModel::autoSelectServer(&client, servers, provider);
            if (taskWeakPtr.expired()) // cancelled and removed from tasks
                return;

            {
                QMutexLocker locker(&mutex);
                auto task = taskWeakPtr.lock();
                if (playInfo.sources.isEmpty()) {
                    qDebug() << "Downloader no links found for" << episodeName;
                    removeTask(task);
                    return;
                }
                auto videoToDownload = playInfo.sources.first();
                qDebug() << videoToDownload.videoUrl.toString();
                task->link = videoToDownload.videoUrl.toString();
                task->headers = videoToDownload.getHeaders();
                task->setProgressText("Awaiting to start...");
                qDebug() << "Log (Downloader) : Starting download for" << episodeName;
                tasksQueue.enqueue(taskWeakPtr);
            }
        }
        startTasks();
        playlist->disuse();
        // ErrorHandler::instance().show (ex.what(), "Download Error");

    });

}


void DownloadManager::runTask(std::shared_ptr<DownloadTask> task) {
    if (!DownloadTask::checkDependencies()) return;
    QProcess process;
    process.setProgram (DownloadTask::N_m3u8DLPath);
    process.setArguments (task->getArguments());
    process.start();
    static QRegularExpression percentRegex = QRegularExpression(R"((\d+\.\d+)%)");
    int percent = 0;
    while (process.state() == QProcess::Running
           && process.waitForReadyRead()
           && !task->isCancelled())
    {
        auto line = process.readAll().trimmed();
        line.replace("━", "");
        QRegularExpressionMatch match = percentRegex.match(line);
        if (match.hasMatch()) {
            percent = static_cast<int>(match.captured(1).toFloat());
            task->setProgressValue (percent);
        } else if (line.contains("ERROR:")) {
            ErrorHandler::instance().show (line, "Download Error");
            // break;
        }
        task->setProgressText(line);
        int i = tasks.indexOf(task);
        emit dataChanged(index(i, 0),index(i, 0));
    }
    if (!task->isCancelled()) {
        process.waitForFinished(-1);
    } else {
        process.kill();
    }
}




void DownloadManager::removeTask(std::shared_ptr<DownloadTask> &task) {
    QMutexLocker locker(&mutex);
    auto index = tasks.indexOf(task);
    Q_ASSERT(index != -1);
    qDebug() << "Log (Downloader) : Removing task" << task->displayName;
    if (auto taskWatcher = task->watcher; taskWatcher) {
        // task has started, not in task queue
        Q_ASSERT(task == watcherTaskTracker[task->watcher]);
        if (taskWatcher->isRunning()){
            qDebug() << "Log (Downloader) : Attempting to cancel the ongoing task" << task->displayName;
            task->cancel();
            task->setProgressText("Cancelling");
            return;
        } else {
            watcherTaskTracker[taskWatcher] = nullptr;
        }
    }

    beginRemoveRows(QModelIndex(), index, index);
    tasks.removeAt(index);
    endRemoveRows();
}

void DownloadManager::watchTask(QFutureWatcher<void> *watcher)
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
    watcher->setFuture (QtConcurrent::run (&DownloadManager::runTask, this, taskPtr));
}




void DownloadManager::cancelTask(int index) {
    if (index >= 0 && index < tasks.size()) {
        removeTask(tasks[index]);
    }
}

void DownloadManager::cancelAllTasks() {
    QMutexLocker locker(&mutex);
    tasksQueue.clear();
    for (int i = tasks.size() - 1; i >= 0; --i) {
        removeTask(tasks[i]);
    }
}

void DownloadManager::startTasks() {
    QMutexLocker locker(&mutex);
    for (auto* watcher:watchers) {
        if (tasksQueue.isEmpty()) break;
        else if (!watcherTaskTracker[watcher]) watchTask(watcher); //if watcher not working on a task
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
        return task->getProgressValue();
        break;
    case ProgressTextRole:
        return task->getProgressText();
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
