#ifndef DOWNLOADMODEL_H
#define DOWNLOADMODEL_H

#include <QDir>
#include <QObject>
#include <QProcess>
#include <QRegularExpression>
#include <QString>
#include <QThread>
#include <QThreadPool>


class DownloadTask : public QObject,public QRunnable {
    Q_OBJECT
    QProcess* process;
public:
    explicit DownloadTask(const QString& videoName, const QString& folder, const QString& link, const QString& headers)
        : m_videoName(videoName), m_folder(folder), m_link(link), m_headers(headers)
    {
    }

    ~DownloadTask(){
        if (process->state() == QProcess::Running) {
            process->kill();
        }
    }
    void cancel() {
        m_cancelled = true;
    }
    void run() override {
        QString nilaodaPath = "C:/Users/Jeffx/Desktop/Kyokou/build/N_m3u8DL-CLI_v3.0.2.exe";
        QStringList m_command = {m_link,"--workDir",m_folder,"--saveName", m_videoName, "--enableDelAfterDone", "--disableDateInfo", "--noProxy"};

        process = new QProcess;
        process->setProgram(nilaodaPath);
        process->setArguments(m_command);

        QRegularExpression re(R"(\d+\.\d+(?=\%))");

        process->start();

        while (process->state() == QProcess::Running && process->waitForReadyRead() && !m_cancelled) {
            auto line = process->readAllStandardOutput().trimmed();
            QRegularExpressionMatch match = re.match(line);
            if (match.hasMatch()) {
                QString percentStr = match.captured();
                double percent = percentStr.toDouble();
                emit progress(percent);
            } else {
                qDebug() << line;
                qDebug() << "No percentage found";
                if (line.contains("Task Done")) {
                    break;
                }
            }
        }

        emit finished();
        qDebug() << "Task ended";
    }

signals:
    void progress(int value);
    void finished();

private:
    QString m_videoName;
    QString m_folder;
    QString m_link;
    QString m_headers;
    bool m_cancelled = false;
};


class DownloadModel: public QObject
{
    Q_OBJECT
public:
    DownloadModel(){
        pool.setMaxThreadCount(4);
    }
    QThreadPool pool;
    QVector<DownloadTask*> tasks;

    void downloadM3u8(QString videoName,const QString& folder,const QString& link,const QString& referer) {
        videoName.replace(':', '.');
//        QString videoPath = QDir(folder).filePath(videoName + ".mp4");
        QString headers = "authority:\"AUTHORITY\"|origin:\"https://REFERER\"|referer:\"https://REFERER/\"|user-agent:\"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/102.0.0.0 Safari/537.36\"sec-ch-ua:\"Not A;Brand\";v=\"99\", \"Chromium\";v=\"102\", \"Google Chrome\";v=\"102\"";
        headers.replace("REFERER", referer.isEmpty() ? link.split("https://")[1].split("/")[0] : referer);

        DownloadTask* task = new DownloadTask(videoName, folder, link, headers);
        connect(task, &DownloadTask::finished, task, &QObject::deleteLater);
        int index = tasks.size ();
        connect(task, &DownloadTask::finished, task, [&,index](){
            tasks.removeOne (task);
        });
//                connect(task, &DownloadTask::progress, this, &DownloaderModel::progress);
        pool.start(task);

        tasks.push_back (task);
    }
    void cancelAllTasks() {
        for (auto& task : tasks) {
            task->cancel();
        }
        tasks.clear();
        pool.clear ();
    }
    ~DownloadModel(){
        cancelAllTasks();
    }
    void cancelTask(int index) {
        if (index >= 0 && index < tasks.size()) {
            DownloadTask* task = tasks[index];
            task->cancel();
            tasks.remove(index);
            task->deleteLater();
        }
    }




};

#endif // DOWNLOADMODEL_H
