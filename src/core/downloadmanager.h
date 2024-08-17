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


class ShowData;

class DownloadTask: public QObject {
    Q_OBJECT
public:
    inline static QString N_m3u8DLPath = "";
    inline static QString tempDir = "";
    inline static QString m_ffmpegPath = "";

    static bool init() {
        if (N_m3u8DLPath.isEmpty())
            N_m3u8DLPath = QDir::cleanPath (QCoreApplication::applicationDirPath() + QDir::separator() + "N_m3u8DL-RE.exe");
        if (m_ffmpegPath.isEmpty())
            m_ffmpegPath = QDir::cleanPath (QCoreApplication::applicationDirPath() + QDir::separator() + "ffmpeg.exe");

        bool isWorking = QFileInfo::exists(N_m3u8DLPath) && QFileInfo::exists(m_ffmpegPath);
        if (isWorking && tempDir.isEmpty()) {
            tempDir = QDir::cleanPath(QCoreApplication::applicationDirPath() + QDir::separator() + "temp");
        }
        return isWorking;
    }
    DownloadTask(const QString &videoName, const QString &folder, const QString &link, const QHash<QString, QString>& headers, const QString &displayName , const QString &path)
        : videoName(videoName), folder(folder), link(link), headers(headers), displayName(displayName), path(path) {}

    ~DownloadTask() {
        qDebug() << displayName << "task deleted";
    }
    QString videoName;
    QString folder;
    QString link;
    QHash<QString, QString> headers;
    QString displayName;
    QString path;
    bool success = false;
    QFutureWatcher<void>* watcher = nullptr;
    QStringList getArguments(){
        QStringList args {link,
                            "--save-dir", folder,
                            "--tmp-dir", tempDir,
                            "--save-name", videoName,
                            "--ffmpeg-binary-path",  m_ffmpegPath,
                            "--del-after-done", "--no-date-info",
                            "--auto-select"};
        if (!headers.isEmpty()) {
            for (auto it = headers.begin(); it != headers.end(); ++it) {
                args.append("-H");
                args.append(it.key() + ": " + it.value());
            }
        }
        return args;
    }


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
    bool m_isCancelled = false;
    int m_progressValue = 0;
    QString m_progressText = "";
};

class DownloadManager: public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString workDir READ getWorkDir WRITE setWorkDir NOTIFY workDirChanged)

    QString m_workDir;
    bool m_isWorking = true;

    QRecursiveMutex mutex;
    QRegularExpression folderNameCleanerRegex = QRegularExpression("[/\\:*?\"<>|]");

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
private:
    QList<QFutureWatcher<void>*> watchers;
    QQueue<std::weak_ptr<DownloadTask>> tasksQueue;
    QList<std::shared_ptr<DownloadTask>> tasks;
    QMap<QFutureWatcher<void>*, std::shared_ptr<DownloadTask>> watcherTaskTracker;

    void removeTask(std::shared_ptr<DownloadTask> &task);
    void watchTask(QFutureWatcher<void>* watcher);
    void startTasks();
    void runTask(std::shared_ptr<DownloadTask> task);
    Q_SIGNAL void workDirChanged(void);
private:
    enum {
        NameRole = Qt::UserRole,
        PathRole,
        ProgressValueRole,
        ProgressTextRole
    };
    int rowCount(const QModelIndex &parent) const { return tasks.count(); };
    QVariant data(const QModelIndex &index, int role) const;
    QHash<int, QByteArray> roleNames() const;
};



