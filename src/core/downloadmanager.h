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

#include "downloadtask.h"

class ShowData;

class DownloadManager: public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString workDir READ getWorkDir WRITE setWorkDir NOTIFY workDirChanged)
    QString m_workDir;
    QString N_m3u8DLPath;
    bool m_isWorking {true};

    QList<QFutureWatcher<bool>*> watchers;
    QQueue<DownloadTask*> tasksQueue;
    QList<DownloadTask*> tasks;
    QMap<QFutureWatcher<bool>*,DownloadTask*> watcherTaskTracker;
    QRecursiveMutex mutex;
    QRegularExpression folderNameCleanerRegex = QRegularExpression("[/\\:*?\"<>|]");
    inline static QRegularExpression percentRegex = QRegularExpression(R"(\d+\.\d+(?=\%))");


public:
    explicit DownloadManager(QObject *parent = nullptr);
    ~DownloadManager() {
        cancelAllTasks();
        qDeleteAll(watchers);
    }

    Q_INVOKABLE void downloadLink(const QString &name, const QString &link);
    void downloadShow(ShowData &show, int startIndex, int count);

    void cancelAllTasks(){
        QMutexLocker locker(&mutex);
        for (auto* watcher : watchers) {
            if (!watcherTaskTracker[watcher]) continue;
            watcherTaskTracker[watcher] = nullptr;
            watcher->cancel();
            watcher->waitForFinished();
        }
        qDeleteAll (tasks);
        tasks.clear();
        tasksQueue.clear();
    }
    Q_INVOKABLE void cancelTask(int index) {
        if (index >= 0 && index < tasks.size()) {
            removeTask (tasks[index]);
            emit layoutChanged();
        }
    }

    QString getWorkDir(){ return m_workDir; }
    bool setWorkDir(const QString& path);
private:
    void removeTask(DownloadTask *task);
    void watchTask(QFutureWatcher<bool>* watcher);
    void addTask(DownloadTask *task) {
        QMutexLocker locker(&mutex);
        tasksQueue.enqueue(task);
        tasks.push_back(task);
    }
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



