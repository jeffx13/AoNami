#include "downloadmanager.h"
#include <QtConcurrent/QtConcurrentRun>
#include "player/playlistitem.h"
#include "providers/showprovider.h"
#include "gui/uibridge.h"
#include "app/logger.h"
#include <QStandardPaths>
#include <memory>
#include "gui/models/serverlistmodel.h"
#include "gui/models/downloadlistmodel.h"
#include "app/settings.h"

QString DownloadManager::cleanFolderName(const QString &name) {
    QString cleanedName = name;
    return cleanedName.replace(":", "꞉")
                      .replace("\"", "'")
                      .replace("?", "？")
                      .replace("*", "∗")
                      .replace("|", "｜")
                      .replace("<", "≺")
                      .replace(">", "≻")
                      .replace("/", "∕")
                      .replace("\\", "⧵");
}

DownloadManager::DownloadManager(QObject *parent)
    : ServiceManager(parent)
{
    m_threadPool.setMaxThreadCount(m_maxDownloads);
}

void DownloadManager::downloadLink(const QString &name, const QString &link) {
    if (!DownloadTask::checkDependencies())
        return;

    QString cleanedName = cleanFolderName(name);
    QString path = Settings::instance().downloadDir() + "/" + cleanedName + ".mp4";
    if (QFile::exists(path) || m_ongoingDownloads.contains(path)) {
        oLog() << "Downloader" << "File already exists or already downloading" << path;
        return;
    }
    emit aboutToInsert(tasks.size());
    m_ongoingDownloads.insert(path);
    tasks.push_back(std::make_shared<DownloadTask>(cleanedName, Settings::instance().downloadDir(), link, cleanedName));
    emit inserted();
    m_taskQueue.append(tasks.back());
    startTasks();
}

void DownloadManager::downloadShow(ShowData &show, int startIndex, int endIndex) {
    if (!DownloadTask::checkDependencies())
        return;

    auto playlist = show.getPlaylist();
    if (!playlist || !playlist->isValidIndex(startIndex))
        return;

    QString showName = cleanFolderName(show.title);
    auto provider = show.provider;

    if (endIndex < startIndex)
        std::swap(startIndex, endIndex);

    if (endIndex >= playlist->count())
        endIndex = playlist->count() - 1;

    cLog() << "Downloader" << showName << "from index" << startIndex << "to" << endIndex;
    QString workDir = Settings::instance().downloadDir() + "/" + showName;

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
        emit aboutToInsert(tasks.size());
        tasks.push_back(task);
        emit inserted();
        m_taskQueue.append(tasks.back());
    }
    startTasks();
}

QString DownloadTask::extractLink() {
    if (!m_provider || !m_episode || m_isCancelled)
        return nullptr;

    setProgressText("Extracting source...");
    Client client(&m_isCancelled);

    auto servers = m_provider->loadServers(&client, m_episode);
    if (m_isCancelled)
        return nullptr;

    auto res = ServerListModel::findWorkingServer(&client, m_provider, servers);
    if (res.first == -1 || m_isCancelled)
        return nullptr;

    PlayInfo &playItem = res.second;
    auto video = playItem.videos.first();

    link = video.url.toString();
    headers = playItem.headers;

    setProgressText("Extracted source successfully!");
    if (auto parent = m_episode->parent())
        parent->disuse();
    m_episode = nullptr;
    m_provider = nullptr;

    return link;
}

void DownloadManager::runTask(std::shared_ptr<DownloadTask> task) {
    if (!DownloadTask::checkDependencies())
        return;

    if (task->link.isEmpty()) {
        task->link = task->extractLink();
        if (task->link.isEmpty())
            return;
    }

    QProcess *process = new QProcess(nullptr);
    process->setProgram(DownloadTask::N_m3u8DLPath);
    process->setArguments(task->getArguments());
    process->setProcessChannelMode(QProcess::MergedChannels);
    task->setProcess(process);
    task->setRunning(true);
    process->start();

    static QRegularExpression percentRegex(R"((\d+\.\d+)%)");
    int percent = 0;

    while (process->state() == QProcess::Running
           && process->waitForReadyRead()
           && !task->isCancelled())
    {
        auto line = process->readAll().trimmed();
        line.replace("━", "");
        QRegularExpressionMatch match = percentRegex.match(line);
        if (match.hasMatch()) {
            percent = static_cast<int>(match.captured(1).toFloat());
            task->setProgressValue(percent);
        } else if (line.contains("ERROR:")) {
            const QString msg = QString("%1\n%2").arg(task->displayName, line);
            QMetaObject::invokeMethod(&UiBridge::instance(), [msg]() {
                UiBridge::instance().showError(msg, "Download Error");
            }, Qt::QueuedConnection);
        }
        task->setProgressText(line);
        int i = tasks.indexOf(task);
        emit dataChanged(i);
    }

    if (!task->isCancelled()) {
        process->waitForFinished(-1);
    } else {
        process->kill();
        process->waitForFinished(-1);
    }

    {
        QMutexLocker locker(&mutex);
        m_currentConcurrentDownloads--;
    }

    task->setRunning(false);
    delete process;
    task->setProcess(nullptr);
    QMetaObject::invokeMethod(this, [this, task]() {
        removeTask(task);
        startTasks();
    }, Qt::QueuedConnection);
}

void DownloadManager::removeTask(const std::shared_ptr<DownloadTask> &task) {
    QMutexLocker locker(&mutex);
    int idx = tasks.indexOf(task);
    Q_ASSERT(idx != -1);

    if (auto proc = task->process(); proc && proc->state() == QProcess::Running) {
        cLog() << "Downloader" << "Attempting to cancel task" << task->displayName;
        task->cancel();
        task->setProgressText("Cancelling");
        return;
    }

    m_ongoingDownloads.remove(task->path);
    emit aboutToRemove(idx);
    tasks.removeAt(idx);
    emit removed();
}

// removed watcher-based scheduling

void DownloadManager::cancelTask(int index) {
    if (index >= 0 && index < tasks.size()) {
        removeTask(tasks[index]);
    }
}

void DownloadManager::cancelAllTasks() {
    QMutexLocker locker(&mutex);
    m_taskQueue.clear();
    for (int i = tasks.size() - 1; i >= 0; --i) {
        removeTask(tasks[i]);
    }
}

void DownloadManager::startTasks() {
    QMutexLocker locker(&mutex);
    while (!m_taskQueue.isEmpty() && m_currentConcurrentDownloads < m_maxDownloads) {
        auto task = m_taskQueue.takeFirst();
        m_currentConcurrentDownloads++;
        auto future = QtConcurrent::run(&m_threadPool, &DownloadManager::runTask, this, task);
    }
}

int DownloadManager::maxDownloads() const {
    return m_maxDownloads;
}

void DownloadManager::setMaxDownloads(int newMaxDownloads) {
    if (m_maxDownloads == newMaxDownloads)
        return;
    m_maxDownloads = newMaxDownloads;
    m_threadPool.setMaxThreadCount(m_maxDownloads);
    emit maxDownloadsChanged();
    startTasks();
}
