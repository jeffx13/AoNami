#include "downloadmanager.h"
#include <QtConcurrent>
#include "data/playlistitem.h"
#include "data/showdata.h"
#include "Providers/showprovider.h"
#include "utils/errorhandler.h"


DownloadManager::DownloadManager(QObject *parent): QAbstractListModel(parent) {
    N_m3u8DLPath = QDir::cleanPath (QCoreApplication::applicationDirPath() + QDir::separator() + "N_m3u8DL-CLI_v3.0.2.exe");
    //m_workDir = QDir::cleanPath (QCoreApplication::applicationDirPath() + QDir::separator() + "Downloads");
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
                }
            }
            removeTask(watcherTaskTracker[watcher]);
            watchTask (watcher);
            emit layoutChanged();
        });

        QObject::connect (watcher, &QFutureWatcher<bool>::progressValueChanged, this, [watcher, this](){
            Q_ASSERT(watcherTaskTracker[watcher]);
            watcherTaskTracker[watcher]->progressValue = watcher->progressValue();
            watcherTaskTracker[watcher]->progressText = watcher->progressText();
            int i = tasks.indexOf (watcherTaskTracker[watcher]);
            emit dataChanged (index(i, 0),index(i, 0));
        });

    }
}

void DownloadManager::removeTask(DownloadTask *task)
{
    QMutexLocker locker(&mutex);
    if (task->watcher) {
        // task has started, not in task queue
        Q_ASSERT(task == watcherTaskTracker[task->watcher]);
        if (task->watcher->isRunning()){
            qDebug() << "Log (Downloader) : Attempting to cancel the ongoing task" << task->displayName;
            task->watcher->cancel();
            task->watcher->waitForFinished();
            return; // this function is called again from the canceled signal slot
        }
        watcherTaskTracker[task->watcher] = nullptr;
    }
    else {
        tasksQueue.removeOne (task);
    }
    qDebug() << "Log (Downloader) : Removed task" << task->displayName;
    tasks.removeOne (task);
    delete task;
}

void DownloadManager::watchTask(QFutureWatcher<bool> *watcher)
{
    QMutexLocker locker(&mutex);
    if (tasksQueue.isEmpty())
        return;
    DownloadTask *task = tasksQueue.front();
    task->watcher = watcher;
    watcherTaskTracker[watcher] = task;
    QStringList command {task->link,"--workDir", task->folder,"--saveName", task->videoName, "--enableDelAfterDone", "--disableDateInfo"};
    watcher->setFuture (QtConcurrent::run (&DownloadManager::executeCommand, this, command));
    tasksQueue.pop_front();
}

void DownloadManager::downloadLink(const QString &name, const QString &link) {
    if (link.isEmpty()) {
        qDebug() << "Log (Downloader) : Empty link!";
        return;
    }
    if (name.isEmpty()){
        qDebug() << "Log (Downloader) : No filename provided!";
        return;
    }
    QString headers = "authority:\"AUTHORITY\"|origin:\"https://REFERER\"|referer:\"https://REFERER/\"|user-agent:\"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/102.0.0.0 Safari/537.36\"sec-ch-ua:\"Not A;Brand\";v=\"99\", \"Chromium\";v=\"102\", \"Google Chrome\";v=\"102\"";
    DownloadTask *task = new DownloadTask(name, m_workDir, link, headers, name , link);
    addTask (task);
    startTasks();
    emit layoutChanged();
}

void DownloadManager::executeCommand(QPromise<bool> &promise, const QStringList &command) {
    promise.setProgressRange (0, 100);
    QProcess process;
    process.setProgram (N_m3u8DLPath);
    process.setArguments (command);
    QRegularExpression re = DownloadManager::percentRegex;
    process.start();
    int percent;
    while (process.state() == QProcess::Running
           && process.waitForReadyRead()
           && !promise.isCanceled()) {
        auto line = process.readAllStandardOutput().trimmed();
        QRegularExpressionMatch match = re.match(line);
        if (match.hasMatch()) {
            percent = match.captured().toDouble();
            promise.setProgressValueAndText (percent, line);
        }
        else if (line.contains("Invalid Uri")) {
            promise.addResult (false); //todo add reason
            return;
        }
    }
    if (process.state() == QProcess::Running) {
        process.kill();
    }
    promise.addResult (true);
}

void DownloadManager::downloadShow(ShowData &show, int startIndex, int count)
{
    auto playlist = show.getPlaylist();
    if (!playlist || count < 1) return;

    playlist->use(); // prevents the playlist from being delete whilst using it
    QString showName = QString(show.title).replace(":",".").replace(folderNameCleanerRegex, "_");   //todo check replace
    auto provider = show.getProvider();
    int endIndex = startIndex + count;
    if (endIndex > playlist->size()) endIndex = playlist->size();
    qDebug() << "Log (Downloader)" << showName << "from index" << startIndex << "to" << endIndex - 1;

    QFuture<void> future = QtConcurrent::run([this, showName, playlist, provider , startIndex, endIndex](){
        try {
            for (int i = startIndex; i < endIndex; ++i) {
                const PlaylistItem* episode = playlist->at (i);
                QList<VideoServer> servers = provider->loadServers(episode);
                QList<Video> videos;
                for (auto &server : servers) {
                    auto source = provider->extractSource (servers.first());
                    if (!source.isEmpty()) {
                        videos = source;
                        break;
                    }
                }
                if (videos.isEmpty()) {
                    qDebug() << "Downloader no links found";
                }
                auto videoToDownload = videos.first();

                auto workDir = QDir::cleanPath (m_workDir + QDir::separator() + showName);
                QString displayName = showName + " : " + episode->getFullName();
                QString path = QDir::cleanPath (workDir + QDir::separator() + episode->getFullName() + ".mp4");
                QString headers = videoToDownload.getHeaders (":", "|", true);
                headers += "|user-agent:\"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/102.0.0.0 Safari/537.36\"sec-ch-ua:\"Not A;Brand\";v=\"99\", \"Chromium\";v=\"102\", \"Google Chrome\";v=\"102\"";
                DownloadTask *task = new DownloadTask(episode->getFullName(), workDir, videoToDownload.videoUrl.toString(), headers, displayName , path);
                qDebug() << "Log (Downloader) : Appending new download task for" << episode->getFullName();
                addTask (task);
            }
            startTasks();
            playlist->disuse();
            emit layoutChanged();
        }catch(const QException &ex) {
            ErrorHandler::instance().show (ex.what(), "Download Error");
        }

    });

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
    DownloadTask* task = tasks.at (index.row());

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
    names[NameRole] = "name";
    names[PathRole] = "path";
    names[ProgressValueRole] = "progressValue";
    names[ProgressTextRole] = "progressText";
    return names;
}
