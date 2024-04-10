#pragma once
#include <QString>
#include <QDebug>
#include <QFutureWatcher>

struct DownloadTask
{
    QString videoName;
    QString folder;
    QString link;
    QString headers;
    QString displayName;
    QString path;
    bool success = false;
    int progressValue = 0;
    QString progressText = "";
    QFutureWatcher<bool>* watcher = nullptr;
    enum State
    {
        SUCCESS,
        FAILED,
        INVALID_URI
    };

    ~DownloadTask()
    {
        qDebug() << displayName << "task deleted";
    }
};
