#pragma once



#include <QAbstractListModel>
#include <QDir>
#include <QProcess>
#include <QRegularExpression>
#include <QString>
#include <QCoreApplication>
#include <QFutureWatcher>
#include <QMap>
#include <QQueue>
#include "gui/serverlistmodel.h"
#include "app/logger.h"
#include "providers/showprovider.h"

class ShowData;

class DownloadTask: public QObject {
    Q_OBJECT
public:
    inline static QString N_m3u8DLPath;
    // inline static QString tempDir;
    inline static QString m_ffmpegPath;
    static bool checkDependencies() {
        if (N_m3u8DLPath.isEmpty()) {
            N_m3u8DLPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + QDir::separator() + "N_m3u8DL-RE.exe");
            // tempDir = QDir::cleanPath(QCoreApplication::applicationDirPath() + QDir::separator() + "temp");
            m_ffmpegPath = QDir::cleanPath (QCoreApplication::applicationDirPath() + QDir::separator() + "ffmpeg.exe");
            // QDir dir(tempDir);
            // if (dir.exists()) {
            //     dir.removeRecursively();
            // }
        }
        return QFileInfo::exists(N_m3u8DLPath) && QFileInfo::exists(m_ffmpegPath);
    }

    DownloadTask(const QString &videoName, const QString &folder, const QString &link,
                 const QString &displayName, const QMap<QString, QString>& headers = QMap<QString, QString>()
                 )
        : videoName(videoName), folder(folder), link(link), displayName(displayName), headers(headers){
        path = QDir::cleanPath(folder + QDir::separator() + videoName + ".mp4");
    }
    DownloadTask(PlaylistItem *episode, ShowProvider *provider, const QString &workDir)
        : m_provider(provider), m_episode(episode)
    {
        episode->parent()->use();
        QString showName = episode->parent()->name;
        videoName = episode->getFullName().trimmed().replace("\n", ". ");
        displayName = showName + " : " + videoName;
        path = QDir::cleanPath(workDir + QDir::separator() + videoName + ".mp4");
        folder = workDir;
    }

    ~DownloadTask() {
        cLog() << displayName << "task deleted";
    }
    QString videoName;
    QString folder;
    QString link;
    QMap<QString, QString> headers;
    QString displayName;
    QString path;
    bool success = false;
    QFutureWatcher<void>* watcher = nullptr;

    PlaylistItem *m_episode;
    ShowProvider *m_provider;

    QStringList getArguments(){
        QStringList args {link,
            "--save-dir", folder,
            "--tmp-dir", folder,
            "--save-name", videoName,
            "--ffmpeg-binary-path",  m_ffmpegPath,
            "--del-after-done", "--no-date-info", "--no-log",
            "--auto-select", "--no-ansi-color"
        };
        if (!headers.isEmpty()) {
            for (auto it = headers.begin(); it != headers.end(); ++it) {
                args.append("-H");
                args.append(it.key() + ": " + it.value());
            }
        }
        return args;
    }

    QString extractLink();


    Q_SIGNAL void progressValueChanged();
    Q_SIGNAL void progressTextChanged();

    int getProgressValue() const {
        return m_progressValue;
    }
    QString getProgressText() const {
        return m_progressText;
    }
    void setProgressValue(int value) {
        if (m_progressValue == value) return;
        m_progressValue = value;
        emit progressValueChanged();
    }
    void setProgressText(const QString &text) {
        m_progressText = text;
        emit progressTextChanged();
    }
    bool isCancelled() const {
        return m_isCancelled;
    }
    void cancel() {
        m_isCancelled = true;
    }
private:
    std::atomic<bool> m_isCancelled = false;
    int m_progressValue = 0;
    QString m_progressText = "Awaiting to start...";
};

class DownloadManager: public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString workDir READ getWorkDir WRITE setWorkDir NOTIFY workDirChanged)
    Q_PROPERTY(int m_maxDownloads READ maxDownloads WRITE setMaxDownloads NOTIFY maxDownloadsChanged FINAL)

public:
    explicit DownloadManager(QObject *parent = nullptr);
    ~DownloadManager() {
        cancelAllTasks();
        qDeleteAll(watchers);
    }

    Q_INVOKABLE void downloadLink(const QString &name, const QString &link);
    void downloadShow(ShowData &show, int startIndex, int count);
    void cancelAllTasks();
    Q_INVOKABLE void cancelTask(int index);
    QString getWorkDir(){ return m_workDir; }
    bool setWorkDir(const QString& path);
    int maxDownloads() const;
    void setMaxDownloads(int newMaxDownloads);

private:
    int m_maxDownloads = 4;
    std::atomic<int> m_currentConcurrentDownloads = 0;
    QString m_workDir;
    QRecursiveMutex mutex;
    QSet<QString> m_ongoingDownloads;
    QList<QFutureWatcher<void>*> watchers;
    QQueue<std::weak_ptr<DownloadTask>> tasksQueue;
    QList<std::shared_ptr<DownloadTask>> tasks;
    QMap<QFutureWatcher<void>*, std::shared_ptr<DownloadTask>> watcherTaskTracker;


    QString cleanFolderName(const QString &name);
    void removeTask(std::shared_ptr<DownloadTask> &task);
    void watchTask(QFutureWatcher<void>* watcher);
    void startTasks();
    void runTask(std::shared_ptr<DownloadTask> task);
    Q_SIGNAL void workDirChanged(void);
    Q_SIGNAL void maxDownloadsChanged();
private:
    enum {
        NameRole = Qt::UserRole,
        PathRole,
        ProgressValueRole,
        ProgressTextRole
    };
    int rowCount(const QModelIndex &parent) const { return tasks.count(); }
    QVariant data(const QModelIndex &index, int role) const;
    QHash<int, QByteArray> roleNames() const;
};



