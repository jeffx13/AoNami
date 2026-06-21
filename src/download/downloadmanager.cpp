#include "downloadmanager.h"
#include <QtConcurrent/QtConcurrentRun>
#include <QDateTime>
#include <QRegularExpression>
#include "player/playlistitem.h"
#include "providers/showprovider.h"
#include "ui/uibridge.h"
#include "app/logger.h"
#include "player/serverselector.h"
#include "app/settings.h"

DownloadTask::DownloadTask(const QString &videoName, const QString &folder, const QString &link,
                           const QString &displayName, const QMap<QString, QString> &headers)
    : videoName(videoName), folder(folder), link(link), headers(headers), displayName(displayName)
{
    path = QDir::cleanPath(folder + "/" + videoName + ".mp4");
}

DownloadTask::DownloadTask(QSharedPointer<PlaylistItem> episode, ShowProvider *provider, const QString &workDir)
    : m_episode(episode), m_provider(provider)
{
    if (episode && episode->parent()) {
        QString showName = episode->parent()->name;
        videoName = DownloadManager::cleanFolderName(episode->displayName.trimmed().replace("\n", ". "));
        displayName = showName + " : " + videoName;
        path = QDir::cleanPath(workDir + "/" + videoName + ".mp4");
        folder = workDir;
    }
}

bool DownloadTask::checkDependencies() {
    ensurePaths();
    return QFile::exists(s_m3u8dlPath) && QFile::exists(s_ffmpegPath);
}

QStringList DownloadTask::getArguments() const {
    QStringList args {
        link,
        "--save-dir", folder,
        "--tmp-dir", folder,
        "--save-name", videoName,
        "--ffmpeg-binary-path", s_ffmpegPath,
        "--del-after-done", "--no-date-info", "--no-log",
        "--auto-select", "--no-ansi-color"
    };
    const QString maxSpeed = Settings::instance().get(Config::MaxSpeed);
    if (!maxSpeed.isEmpty())
        args << "--max-speed" << maxSpeed;
    for (auto it = headers.constBegin(); it != headers.constEnd(); ++it)
        args << "-H" << (it.key() + ": " + it.value());
    return args;
}

QStringList DownloadTask::getFfmpegArguments() const {
    // ffmpeg -headers applies to the input that follows it, so emit it before each.
    QString headerBlock;
    for (auto it = headers.constBegin(); it != headers.constEnd(); ++it)
        headerBlock += it.key() + ": " + it.value() + "\r\n";

    QStringList args { "-y", "-hide_banner" };
    if (!headerBlock.isEmpty()) args << "-headers" << headerBlock;
    args << "-i" << link;
    if (!headerBlock.isEmpty()) args << "-headers" << headerBlock;
    args << "-i" << audioLink
         << "-map" << "0:v:0" << "-map" << "1:a:0"
         << "-c" << "copy" << "-movflags" << "+faststart"
         << path;
    return args;
}

QString DownloadTask::extractLink() {
    if (!m_provider || !m_episode || m_cancel.isCancelled())
        return {};

    setProgressText("Extracting source...");
    Client client(m_cancel);

    auto servers = m_provider->loadServers(&client, m_episode.data());
    if (m_cancel.isCancelled()) return {};

    auto res = ServerSelector::findWorkingServer(&client, m_provider, servers);
    if (!res.found() || m_cancel.isCancelled()) return {};

    link = res.playInfo.videos.first().url.toString();
    if (!res.playInfo.audios.isEmpty())
        audioLink = res.playInfo.audios.first().url.toString();
    headers = res.playInfo.headers;
    setProgressText("Extracted source successfully!");

    m_episode = nullptr;
    m_provider = nullptr;
    return link;
}

void DownloadTask::setProgressValue(int value) {
    // Marshal onto the object's thread - main-thread Q_PROPERTY reads race otherwise.
    QMetaObject::invokeMethod(this, [this, value]() {
        if (m_progressValue == value) return;
        m_progressValue = value;

        // ETA from the average completion rate - format-independent.
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (m_startTimeMs == 0) m_startTimeMs = now;
        double elapsed = (now - m_startTimeMs) / 1000.0;
        if (value > 0 && value < 100 && elapsed > 2.0) {
            double rate = value / elapsed;   // percent per second
            if (rate > 0) m_etaText = formatEta(static_cast<int>((100 - value) / rate));
        } else {
            m_etaText.clear();
        }
        rebuildStats();
        emit progressValueChanged();
    }, Qt::QueuedConnection);
}

void DownloadTask::setProgressText(const QString &text) {
    QMetaObject::invokeMethod(this, [this, text]() {
        if (m_progressText == text) return;
        m_progressText = text;
        emit progressTextChanged();
    }, Qt::QueuedConnection);
}

void DownloadTask::setSpeed(const QString &speed) {
    QMetaObject::invokeMethod(this, [this, speed]() {
        if (m_speed == speed) return;
        m_speed = speed;
        rebuildStats();
    }, Qt::QueuedConnection);
}

void DownloadTask::resetStats() {
    QMetaObject::invokeMethod(this, [this]() {
        m_startTimeMs = 0;
        m_speed.clear();
        m_etaText.clear();
        m_stats.clear();
    }, Qt::QueuedConnection);
}

void DownloadTask::rebuildStats() {
    QStringList parts;
    if (!m_speed.isEmpty())   parts << m_speed;
    if (!m_etaText.isEmpty()) parts << ("ETA " + m_etaText);
    m_stats = parts.join(QStringLiteral("  •  "));
}

QString DownloadTask::formatEta(int seconds) {
    if (seconds < 0) seconds = 0;
    int h = seconds / 3600, m = (seconds % 3600) / 60, s = seconds % 60;
    if (h > 0)
        return QString("%1:%2:%3").arg(h).arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
    return QString("%1:%2").arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
}

QString DownloadManager::cleanFolderName(const QString &name) {
    static const QList<QPair<QChar, QChar>> replacements = {
        {':', u'꞉'}, {'"', '\''}, {'?', u'？'}, {'*', u'∗'},
        {'|', u'｜'}, {'<', u'≺'}, {'>', u'≻'}, {'/', u'∕'}, {'\\', u'⧵'}
    };
    QString result = name;
    for (const auto &[from, to] : replacements)
        result.replace(from, to);
    // Strip control characters (newlines/tabs) that Windows rejects.
    result.remove(QRegularExpression(QStringLiteral("[\\x00-\\x1F]")));
    // Windows rejects names ending in a space or dot (-> "Access denied").
    while (!result.isEmpty() && (result.endsWith(' ') || result.endsWith('.')))
        result.chop(1);
    result = result.trimmed();
    return result.isEmpty() ? QStringLiteral("download") : result;
}

DownloadManager::DownloadManager(QObject *parent)
    : ListModel(parent)
{
    m_threadPool.setMaxThreadCount(m_maxDownloads);
}

int DownloadManager::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : m_tasks.count();
}

QVariant DownloadManager::data(const QModelIndex &index, int role) const {
    auto *task = taskAt(index.row());
    if (!task) return {};
    switch (role) {
    case NameRole:          return task->displayName;
    case PathRole:          return task->path;
    case ProgressValueRole: return task->getProgressValue();
    case ProgressTextRole:  return task->getProgressText();
    case StatusRole:        return task->status();
    case StatsRole:         return task->getStats();
    default:                return {};
    }
}

QHash<int, QByteArray> DownloadManager::roleNames() const {
    return {
        {NameRole, "downloadName"}, {PathRole, "downloadPath"},
        {ProgressValueRole, "progressValue"}, {ProgressTextRole, "progressText"},
        {StatusRole, "status"}, {StatsRole, "stats"},
    };
}

void DownloadManager::emitRowChanged(int row) {
    if (row < 0) return;
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, [this, row]() {
            if (row < m_tasks.size()) { auto i = index(row); emit dataChanged(i, i); }
        }, Qt::QueuedConnection);
    } else if (row < m_tasks.size()) {
        auto i = index(row);
        emit dataChanged(i, i);
    }
}

void DownloadManager::downloadLink(const QString &name, const QString &link) {
    if (!DownloadTask::checkDependencies()) return;

    QString cleanedName = cleanFolderName(name);
    QString path = Settings::instance().downloadDir() + "/" + cleanedName + ".mp4";
    if (QFile::exists(path) || m_ongoingPaths.contains(path)) {
        oLog() << "Downloader" << "Already exists or downloading" << path;
        return;
    }

    m_ongoingPaths.insert(path);
    beginInsertRows(QModelIndex(), m_tasks.size(), m_tasks.size());
    m_tasks.push_back(QSharedPointer<DownloadTask>::create(cleanedName, Settings::instance().downloadDir(), link, cleanedName));
    endInsertRows();
    m_taskQueue.append(m_tasks.back());
    startTasks();
}

void DownloadManager::downloadShow(ShowData &show, int startIndex, int endIndex) {
    if (!DownloadTask::checkDependencies()) return;

    auto playlist = show.getPlaylist();
    if (!playlist || !playlist->isValidIndex(startIndex)) return;

    if (endIndex < startIndex) std::swap(startIndex, endIndex);
    endIndex = qMin(endIndex, playlist->count() - 1);

    QString showName = cleanFolderName(show.title);
    QString workDir = Settings::instance().downloadDir() + "/" + showName;
    cLog() << "Downloader" << showName << "from" << startIndex << "to" << endIndex;

    // Main thread: no mutex needed, and signals must not be emitted under a lock.
    for (int i = startIndex; i <= endIndex; ++i) {
        auto task = QSharedPointer<DownloadTask>::create(playlist->at(i), show.provider, workDir);
        if (QFile::exists(task->path) || m_ongoingPaths.contains(task->path)) {
            cLog() << "Downloader" << "Already exists or downloading" << task->path;
            continue;
        }
        m_ongoingPaths.insert(task->path);
        beginInsertRows(QModelIndex(), m_tasks.size(), m_tasks.size());
        m_tasks.push_back(task);
        endInsertRows();
        m_taskQueue.append(task);
    }
    startTasks();
}

void DownloadManager::runTask(QSharedPointer<DownloadTask> task) {
    if (!DownloadTask::checkDependencies()) {
        // startTasks() already counted this slot - release it so downloads don't stall.
        { QMutexLocker locker(&m_mutex); m_currentConcurrentDownloads--; }
        QMetaObject::invokeMethod(this, [this]() { startTasks(); }, Qt::QueuedConnection);
        return;
    }

    if (task->link.isEmpty()) {
        task->link = task->extractLink();
        if (task->link.isEmpty()) {
            { QMutexLocker locker(&m_mutex); m_currentConcurrentDownloads--; }
            QMetaObject::invokeMethod(this, [this, task]() {
                task->setStatus(DownloadTask::Failed);
                task->setProgressText("Extraction failed - press Retry");
                int row; { QMutexLocker locker(&m_mutex); row = m_tasks.indexOf(task); }
                emitRowChanged(row);
                startTasks();
            }, Qt::QueuedConnection);
            return;
        }
    }

    const bool ffmpeg = task->usesFfmpeg();
    if (ffmpeg) QDir().mkpath(task->folder);   // N_m3u8DL-RE makes its save-dir; ffmpeg won't

    auto *process = new QProcess(nullptr);
    process->setProgram(task->program());
    process->setArguments(ffmpeg ? task->getFfmpegArguments() : task->getArguments());
    process->setProcessChannelMode(QProcess::MergedChannels);
    task->setProcess(process);
    task->setRunning(true);
    process->start();

    static QRegularExpression percentRegex(R"((\d+\.\d+)%)");
    static QRegularExpression speedRegex(R"(([\d.]+\s*[KMGTP]?i?B(?:ps|/s)))");
    // ffmpeg has no %, so derive it from `time=` vs the `Duration:` it prints.
    static QRegularExpression ffDurRegex(R"(Duration:\s*(\d+):(\d+):(\d+)\.(\d+))");
    static QRegularExpression ffTimeRegex(R"(time=\s*(\d+):(\d+):(\d+)\.(\d+))");
    auto ffSeconds = [](const QRegularExpressionMatch &m) {
        return m.captured(1).toInt() * 3600 + m.captured(2).toInt() * 60
             + m.captured(3).toInt() + m.captured(4).toInt() / 100.0;
    };
    double ffTotal = -1;

    // Drain stdout continuously (a full pipe blocks the child); also re-checks cancel/pause.
    while (!task->isCancelled() && !task->isPaused()) {
        bool ready = process->waitForReadyRead(1000);
        if (process->bytesAvailable() > 0) {
            auto line = process->readAll().trimmed();
            line.replace("━", "");
            if (ffmpeg) {
                if (ffTotal < 0) {
                    auto dm = ffDurRegex.match(line);
                    if (dm.hasMatch()) ffTotal = ffSeconds(dm);
                }
                double cur = -1;
                for (auto it = ffTimeRegex.globalMatch(line); it.hasNext(); )
                    cur = ffSeconds(it.next());   // newest position in this chunk
                if (cur >= 0 && ffTotal > 0) {
                    task->setProgressValue(qBound(0, int(cur / ffTotal * 100), 100));
                    task->setProgressText("Downloading...");
                }
            } else {
                auto match = percentRegex.match(line);
                if (match.hasMatch())
                    task->setProgressValue(static_cast<int>(match.captured(1).toFloat()));
                else if (line.contains("ERROR:")) {
                    QString msg = QString("%1\n%2").arg(task->displayName, line);
                    QMetaObject::invokeMethod(&UiBridge::instance(), [msg]() {
                        UiBridge::instance().showError(msg, "Download Error");
                    }, Qt::QueuedConnection);
                }
                auto sm = speedRegex.match(line);
                if (sm.hasMatch()) task->setSpeed(sm.captured(1).simplified());
                task->setProgressText(line);
            }
            int i; { QMutexLocker locker(&m_mutex); i = m_tasks.indexOf(task); }
            emitRowChanged(i);
        }
        if (!ready && process->state() != QProcess::Running)
            break;
    }

    const bool cancelled = task->isCancelled();
    const bool paused    = task->isPaused();
    if (cancelled || paused) process->kill();
    process->waitForFinished(-1);

    const bool startFailed = process->error() == QProcess::FailedToStart;
    const bool succeeded = !cancelled && !paused && !startFailed && process->exitCode() == 0;

    { QMutexLocker locker(&m_mutex); m_currentConcurrentDownloads--; }

    task->setRunning(false);
    task->setProcess(nullptr);  // Clear first so concurrent reads see nullptr before we delete
    delete process;

    QMetaObject::invokeMethod(this, [this, task, succeeded, cancelled, paused]() {
        if (cancelled) {
            removeTask(task);
        } else if (paused) {
            task->setPaused(false);
            task->setStatus(DownloadTask::Paused);
            task->setProgressText("Paused");
            int row; { QMutexLocker locker(&m_mutex); row = m_tasks.indexOf(task); }
            emitRowChanged(row);
        } else if (succeeded) {
            UiBridge::instance().showInfo(task->displayName, "Download Complete");
            removeTask(task);
        } else {
            task->setStatus(DownloadTask::Failed);
            task->setProgressText("Failed - press Retry to resume");
            int row; { QMutexLocker locker(&m_mutex); row = m_tasks.indexOf(task); }
            emitRowChanged(row);
        }
        startTasks();
    }, Qt::QueuedConnection);
}

void DownloadManager::removeTask(const QSharedPointer<DownloadTask> &task) {
    // Always called on the main thread (direct from cancelTask, or via QueuedConnection from runTask).
    int idx;
    {
        QMutexLocker locker(&m_mutex);
        idx = m_tasks.indexOf(task);
        if (idx == -1) return;
        if (auto *proc = task->process(); proc && proc->state() == QProcess::Running) {
            // Process still running - cancel it; runTask will call removeTask again when it exits.
            cLog() << "Downloader" << "Cancelling" << task->displayName;
            task->cancel();
            task->setProgressText("Cancelling");
            return;
        }
    }

    beginRemoveRows(QModelIndex(), idx, idx);
    {
        QMutexLocker locker(&m_mutex);
        m_ongoingPaths.remove(task->path);
        m_tasks.removeAt(idx);
    }
    endRemoveRows();
}

void DownloadManager::cancelTask(int index) {
    QSharedPointer<DownloadTask> task;
    { QMutexLocker locker(&m_mutex); if (index >= 0 && index < m_tasks.size()) task = m_tasks[index]; }
    if (task) removeTask(task);
}

void DownloadManager::cancelAllTasks() {
    QList<QSharedPointer<DownloadTask>> copy;
    { QMutexLocker locker(&m_mutex); m_taskQueue.clear(); copy = m_tasks; }
    for (int i = copy.size() - 1; i >= 0; --i)
        removeTask(copy[i]);
}

void DownloadManager::pauseTask(int index) {
    QSharedPointer<DownloadTask> task;
    bool wasQueued = false;
    {
        QMutexLocker locker(&m_mutex);
        if (index < 0 || index >= m_tasks.size()) return;
        task = m_tasks[index];
        if (task->status() == DownloadTask::Queued) {
            m_taskQueue.removeOne(task);
            wasQueued = true;
        }
    }
    if (wasQueued) {
        task->setStatus(DownloadTask::Paused);
        task->setProgressText("Paused");
        emitRowChanged(index);
    } else if (task->status() == DownloadTask::Running) {
        // Worker loop sees isPaused(), kills the process (keeping temp segments); resumes next run.
        task->setPaused(true);
        task->setProgressText("Pausing...");
        emitRowChanged(index);
    }
}

void DownloadManager::resumeTask(int index) {
    QSharedPointer<DownloadTask> task;
    {
        QMutexLocker locker(&m_mutex);
        if (index < 0 || index >= m_tasks.size()) return;
        task = m_tasks[index];
        if (task->status() != DownloadTask::Paused && task->status() != DownloadTask::Failed) return;
        task->setPaused(false);
        task->setStatus(DownloadTask::Queued);
        if (!m_taskQueue.contains(task)) m_taskQueue.append(task);
    }
    task->setProgressText("Queued");
    emitRowChanged(index);
    startTasks();
}

void DownloadManager::startTasks() {
    QList<int> startedRows;
    {
        QMutexLocker locker(&m_mutex);
        while (!m_taskQueue.isEmpty() && m_currentConcurrentDownloads < m_maxDownloads) {
            auto task = m_taskQueue.takeFirst();
            m_currentConcurrentDownloads++;
            task->setStatus(DownloadTask::Running);
            task->resetStats();
            task->setProgressText("Starting...");
            int row = m_tasks.indexOf(task);
            if (row >= 0) startedRows.append(row);
            m_threadPool.start([this, task]() { runTask(task); });
        }
    }
    for (int row : startedRows) emitRowChanged(row);
}

int DownloadManager::maxDownloads() const { return m_maxDownloads; }

void DownloadManager::setMaxDownloads(int n) {
    if (m_maxDownloads == n) return;
    m_maxDownloads = n;
    m_threadPool.setMaxThreadCount(n);
    emit maxDownloadsChanged();
    startTasks();
}
