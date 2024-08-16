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


class DownloadManager: public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString workDir READ getWorkDir WRITE setWorkDir NOTIFY workDirChanged)
    QString m_ffmpegPath;
    struct DownloadTask
    {

        QString videoName;
        QString folder;
        QString link;
        QHash<QString, QString> headers;
        QString displayName;
        QString path;
        bool success = false;
        int progressValue = 0;
        QString progressText = "";
        QFutureWatcher<bool>* watcher = nullptr;
        bool isCancelled = false;
        enum State
        {
            SUCCESS,
            FAILED,
            INVALID_URI
        };
        DownloadTask(const QString &videoName, const QString &folder, const QString &link, const QHash<QString, QString>& headers, const QString &displayName , const QString &path)
            : videoName(videoName), folder(folder), link(link), headers(headers), displayName(displayName), path(path) {}

        ~DownloadTask() {
            qDebug() << displayName << "task deleted";
        }
    };

    QString m_workDir;
    QString N_m3u8DLPath;
    bool m_isWorking {true};

    QList<QFutureWatcher<bool>*> watchers;
    QQueue<std::weak_ptr<DownloadTask>> tasksQueue;
    QList<std::shared_ptr<DownloadTask>> tasks;
    QMap<QFutureWatcher<bool>*, std::shared_ptr<DownloadTask>> watcherTaskTracker;
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
    void removeTask(std::shared_ptr<DownloadTask> &task);
    void watchTask(QFutureWatcher<bool>* watcher);
    void enqueue(std::shared_ptr<DownloadTask> &task);
    void startTasks();
    void executeCommand(QPromise<bool> &promise, const QStringList &command);
signals:
    void workDirChanged(void);
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



