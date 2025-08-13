#include "downloadmanager.h"
#include <QtConcurrent/QtConcurrentRun>
#include "player/playlistitem.h"
#include "providers/showprovider.h"
#include "gui/errordisplayer.h"
#include "app/logger.h"

#include <QStandardPaths>
#include <memory>

#include "gui/serverlistmodel.h"

QString DownloadManager::cleanFolderName(const QString &name) {
    auto cleanedName = name;
    return cleanedName.replace(":","꞉").replace("\"", "'")
        .replace("?", "？").replace("*", "∗")
        .replace("|", "｜").replace("<", "≺").replace(">", "≻")
        .replace("/", "∕").replace("\\", "⧵");
}

DownloadManager::DownloadManager(QObject *parent): QAbstractListModel(parent) {
    m_workDir = QDir::cleanPath(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
    constexpr int maxConcurrentTask = 6 ;

    for (int i = 0; i < maxConcurrentTask; ++i) {
        auto watcher = new QFutureWatcher<void>();
        watchers.push_back(watcher);
        QObject::connect (watcher, &QFutureWatcher<void>::finished, this, [watcher, this](){
            // Set task success
            auto task = watcherTaskTracker[watcher];
            if (!watcher->future().isValid()) {
                gLog() << "Downloader" << task->displayName << "cancelled successfully";
            } else {
                try {
                    watcher->future().waitForFinished();
                } catch (...) {
                    rLog() << "Downloader" << task->displayName << "task failed";
                    ErrorDisplayer::instance().show (QString("Failed to download %1").arg(task->displayName), "Download Error");
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
    if (QFile::exists(path) || m_ongoingDownloads.contains(path)) {
        oLog() << "Downloader" << "File already exists or already downloading" << path;
        return;
    }
    m_ongoingDownloads.insert(path);
    tasks.push_back(std::move(
        std::make_shared<DownloadTask>(name, m_workDir, link, cleanedName)
        ));
    endInsertRows();
    tasksQueue.enqueue(tasks.back());
    startTasks();
}


void DownloadManager::downloadShow(ShowData &show, int startIndex, int endIndex) {
    if (!DownloadTask::checkDependencies()) return;

    auto playlist = show.getPlaylist();
    if (!playlist || !playlist->isValidIndex(startIndex)) return;
    // playlist->use(); // Prevents the playlist from being deleted whilst using it
    QString showName = cleanFolderName(show.title);   //todo check replace
    auto provider = show.provider;
    if (endIndex < startIndex) {
        auto tmp = startIndex;
        startIndex = endIndex;
        endIndex = tmp;
    }
    if (endIndex > playlist->size()) endIndex = playlist->size();
    cLog() << "Downloader" << showName << "from index" << startIndex << "to" << endIndex - 1;
    QString workDir = QDir::cleanPath(m_workDir + QDir::separator() + showName);
    for (int i = startIndex; i <= endIndex; ++i) {
        PlaylistItem* episode = playlist->at(i);
        auto task = std::make_shared<DownloadTask>(episode, provider, workDir);
        if (QFile::exists(task->path) || m_ongoingDownloads.contains(task->path)) {
            cLog() << "Downloader" << "File already exists or already downloading" << task->path;
            continue;
        }
        cLog() << "Downloader" << "Appending new download task for" << task->videoName;
        QMutexLocker locker(&mutex);
        m_ongoingDownloads.insert(task->path);
        beginInsertRows(QModelIndex(), tasks.size(), tasks.size());
        tasks.push_back(std::move(task));
        endInsertRows();
        tasksQueue.enqueue(tasks.back());
    }
    startTasks();
}

QString DownloadTask::extractLink() {
    if (m_provider == nullptr || m_episode == nullptr || m_isCancelled) {
        return nullptr;
    }
    setProgressText("Extracting source...");
    Client client(&m_isCancelled);

    auto servers = m_provider->loadServers(&client, m_episode);
    if (m_isCancelled) return nullptr;

    auto res = ServerListModel::findWorkingServer(&client, m_provider, servers);
    // TODO deal with videos with separate audio files
    if (res.first == -1 || m_isCancelled) return nullptr;

    PlayItem &playItem = res.second;
    auto video = playItem.videos.first();

    link = video.url.toString();
    headers = playItem.headers;

    setProgressText("Extracted source successfully!");
    m_episode->parent()->disuse();
    m_episode = nullptr;
    m_provider = nullptr;

    return link;
}

void DownloadManager::runTask(std::shared_ptr<DownloadTask> task) {
    if (!DownloadTask::checkDependencies()) return;

    if (task->link.isEmpty()) {

        task->link = task->extractLink();
        if (task->link.isEmpty()) return;
    }
    m_currentConcurrentDownloads++;


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
            ErrorDisplayer::instance().show (QString("%1\n%2").arg(task->displayName, line), "Download Error");
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
    QFile::remove(task->link);
    QMutexLocker locker(&mutex);
    m_ongoingDownloads.remove(task->path);
    m_currentConcurrentDownloads--;

}




void DownloadManager::removeTask(std::shared_ptr<DownloadTask> &task) {
    QMutexLocker locker(&mutex);
    auto index = tasks.indexOf(task);
    Q_ASSERT(index != -1);
    if (auto taskWatcher = task->watcher; taskWatcher) {
        // task has started, not in task queue
        Q_ASSERT(task == watcherTaskTracker[task->watcher]);
        if (taskWatcher->isRunning()){
            cLog() << "Downloader" << "Attempting to kill task" << task->displayName;
            task->cancel();
            task->setProgressText("Cancelling");
            return;
        } else {
            watcherTaskTracker[taskWatcher] = nullptr;
        }
    }
    m_ongoingDownloads.remove(task->path);
    beginRemoveRows(QModelIndex(), index, index);
    tasks.removeAt(index);
    endRemoveRows();
}

void DownloadManager::watchTask(QFutureWatcher<void> *watcher) {
    if (m_currentConcurrentDownloads >= m_maxDownloads) return;
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
    for (auto* watcher:std::as_const(watchers)) {
        if (tasksQueue.isEmpty() || m_currentConcurrentDownloads >= m_maxDownloads) break;
        else if (!watcherTaskTracker[watcher]) watchTask(watcher); //if watcher not working on a task
    }
}

bool DownloadManager::setWorkDir(const QString &path) {
    const QFileInfo outputDir(path);
    if ((!outputDir.exists()) || (!outputDir.isDir()) || (!outputDir.isWritable())) {
        oLog() << "Downloader" << "Output directory either doesn't exist or isn't a directory or writeable"
                   << outputDir.absoluteFilePath();
        return false;
    }
    m_workDir = path;

    emit workDirChanged();
    return true;
}

QVariant DownloadManager::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) return {};

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
        return {};
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



int DownloadManager::maxDownloads() const
{
    return m_maxDownloads;
}

void DownloadManager::setMaxDownloads(int newMaxDownloads)
{
    if (m_maxDownloads == newMaxDownloads)
        return;
    m_maxDownloads = newMaxDownloads;
    emit maxDownloadsChanged();
    startTasks();
}
