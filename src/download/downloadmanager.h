#pragma once

#include <QAbstractListModel>
#include <QDir>
#include <QProcess>
#include <QCoreApplication>
#include <QMutex>
#include <QThreadPool>
#include <QSharedPointer>
#include <atomic>

#include "app/logger.h"
#include "core/listmodel.h"

class ShowData;
class PlaylistItem;
class ShowProvider;

// A single download job - a direct URL, or an episode needing source extraction.
class DownloadTask : public QObject {
    Q_OBJECT
public:
    DownloadTask(const QString &videoName, const QString &folder, const QString &link,
                 const QString &displayName, const QMap<QString, QString> &headers = {});

    DownloadTask(QSharedPointer<PlaylistItem> episode, ShowProvider *provider, const QString &workDir);

    ~DownloadTask() override = default;

    enum Status { Queued = 0, Running = 1, Paused = 2, Failed = 3 };

    QString videoName;
    QString folder;
    QString link;
    QString audioLink;   // set when the source has a separate audio stream (e.g. Bilibili DASH)
    QMap<QString, QString> headers;
    QString displayName;
    QString path;

    QStringList getArguments() const;
    // Separate video+audio (Bilibili) isn't a manifest N_m3u8DL-RE can take - ffmpeg muxes both.
    bool usesFfmpeg() const { return !audioLink.isEmpty(); }
    // ffmpeg writes here and we rename on success, so a cancelled run can't leave a stub at `path`
    // (both download entry points skip any task whose `path` already exists). Keep the .mp4
    // suffix - ffmpeg picks the container from the extension.
    QString partPath() const { return QDir::cleanPath(folder + "/" + videoName + ".part.mp4"); }
    QString program() const { return usesFfmpeg() ? ffmpegPath() : toolPath(); }
    QStringList getFfmpegArguments() const;
    QString extractLink();  // Resolve episode -> video URL. Returns empty on failure.
    QString extractLinkInner();

    int getProgressValue() const { return m_progressValue; }
    QString getProgressText() const { return m_progressText; }
    QString getStats() const { return m_stats; }
    void setProgressValue(int value);
    void setProgressText(const QString &text);
    void setSpeed(const QString &speed);
    void resetStats();   // called when (re)starting so ETA recomputes from now

    int  status() const { return m_status.load(); }
    void setStatus(int s) { m_status.store(s); }

    bool isCancelled() const { return m_cancel.isCancelled(); }
    void cancel() { m_cancel.cancel(); }
    bool isPaused() const { return m_isPaused.load(); }
    void setPaused(bool p) { m_isPaused = p; }
    bool isRunning() const { return m_isRunning.load(); }
    void setRunning(bool running) { m_isRunning = running; }
    QProcess *process() const { return m_process.load(std::memory_order_acquire); }
    void setProcess(QProcess *proc) { m_process.store(proc, std::memory_order_release); }

    static bool checkDependencies();
    static QString toolPath()   { ensurePaths(); return s_m3u8dlPath; }
    static QString ffmpegPath() { ensurePaths(); return s_ffmpegPath; }

signals:
    void progressValueChanged();
    void progressTextChanged();

private:
    void rebuildStats();
    static QString formatEta(int seconds);

    static void ensurePaths() {
        if (!s_m3u8dlPath.isEmpty()) return;
        QString appDir = QCoreApplication::applicationDirPath();
        s_m3u8dlPath = QDir::cleanPath(appDir + "/N_m3u8DL-RE.exe");
        s_ffmpegPath = QDir::cleanPath(appDir + "/ffmpeg.exe");
    }

    static inline QString s_m3u8dlPath;
    static inline QString s_ffmpegPath;

    CancelToken       m_cancel;
    std::atomic<bool> m_isPaused{false};
    std::atomic<bool> m_isRunning{false};
    std::atomic<int>  m_status{Queued};
    std::atomic<QProcess*> m_process{nullptr};
    int m_progressValue = 0;
    QString m_progressText = QStringLiteral("Awaiting to start...");

    // Stats (updated on the object's thread)
    QString m_stats;
    QString m_speed;
    QString m_etaText;
    qint64  m_startTimeMs = 0;

    // Only used for episode downloads (cleared after extractLink)
    QSharedPointer<PlaylistItem> m_episode;
    ShowProvider *m_provider = nullptr;
};

class DownloadManager : public ListModel {
    Q_OBJECT
    Q_PROPERTY(int maxDownloads READ maxDownloads WRITE setMaxDownloads NOTIFY maxDownloadsChanged)
public:
    enum Role { NameRole = Qt::UserRole, PathRole, ProgressValueRole, ProgressTextRole, StatusRole, StatsRole };

    explicit DownloadManager(QObject *parent = nullptr);
    ~DownloadManager() override { cancelAllTasks(); m_threadPool.waitForDone(); }

    Q_INVOKABLE void downloadLink(const QString &name, const QString &link);
    void downloadShow(ShowData &show, int startIndex, int endIndex);
    static QString cleanFolderName(const QString &name);  // make a path-safe file/folder name
    Q_INVOKABLE void cancelTask(int index);
    Q_INVOKABLE void pauseTask(int index);
    Q_INVOKABLE void resumeTask(int index);   // also used to retry a failed task
    Q_INVOKABLE void pauseAll();
    Q_INVOKABLE void resumeAll();             // resume paused + retry failed
    Q_INVOKABLE void cancelAll() { cancelAllTasks(); }
    void cancelAllTasks();

    int maxDownloads() const;
    void setMaxDownloads(int newMaxDownloads);

    int count() const { return m_tasks.count(); }
    DownloadTask *taskAt(int i) const {
        return (i >= 0 && i < m_tasks.size()) ? m_tasks.at(i).data() : nullptr;
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

signals:
    void maxDownloadsChanged();

private:
    void startTasks();
    void runTask(QSharedPointer<DownloadTask> task);
    void removeTask(const QSharedPointer<DownloadTask> &task);
    void emitRowChanged(int row);   // safe from any thread
    int  rowOf(const QSharedPointer<DownloadTask> &task) const;

    int m_maxDownloads = 4;
    std::atomic<int> m_currentConcurrentDownloads{0};
    mutable QMutex m_mutex;   // guards the three containers below against the worker threads
    QSet<QString> m_ongoingPaths;
    QList<QSharedPointer<DownloadTask>> m_taskQueue;
    QList<QSharedPointer<DownloadTask>> m_tasks;
    QThreadPool m_threadPool;
};
