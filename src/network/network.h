#pragma once
#include <QDebug>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutex>
#include <QTimer>
#include "curl/curl.h"
#include "csoup.h"
#include "myexception.h"
#include <QJsonArray>

class Client {
public:
    enum RequestType {
        GET,
        POST,
        HEAD,
        PUT,
        DELETE_,
        CONNECT,
        OPTIONS
    };
    struct Response {
        long code;
        // QUrl url;
        QString redirectUrl;
        //        QMap<QString, QString> headers;
        QString headers;
        QString body;
        QMap<QString, QString> cookies;

        QJsonObject toJsonObject(){
            QJsonParseError error;
            QJsonDocument jsonData = QJsonDocument::fromJson(body.toUtf8(), &error);
            if (error.error != QJsonParseError::NoError) {
                qWarning() << "JSON parsing error:" << error.errorString();
                return QJsonObject{};
            }
            return jsonData.object();
        }
        QJsonArray toJsonArray(){
            QJsonParseError error;
            QJsonDocument jsonData = QJsonDocument::fromJson(body.toUtf8(), &error);
            if (error.error != QJsonParseError::NoError) {
                qWarning() << "JSON parsing error:" << error.errorString();
                return {};
            }
            return jsonData.array();
        }
        CSoup toSoup(){
            return CSoup::parse(body);
        }

        ~Response(){}
    };
    Client(std::atomic<bool>* shouldCancel): m_isCancelled(shouldCancel) {
        // QMutexLocker locker(&mutex);
        // if (m_curls.isEmpty()) {
        //     qDebug() << "m_curls is empty";
        //     QTimer timer;
        //     timer.setInterval(100);
        //     QEventLoop loop;
        //     QObject::connect(&timer, &QTimer::timeout, [&]() {
        //         if (!m_curls.isEmpty()) {
        //             qDebug() << "List is no longer empty!";
        //             loop.quit();
        //         }
        //     });
        //     timer.start();
        //     loop.exec();
        // }
        // m_curl = m_curls.last();
        // m_curls.removeLast();


    }
    // ~Client() {
    //     Q_ASSERT(m_curl);
    //     QMutexLocker locker(&mutex);
    //     curl_easy_reset (m_curl);
    //     m_curls.push_back(m_curl);
    //     qDebug() << "returned";
    // }

    bool isOk(const QString& url, long timeout = 5L);
    Response get(const QString &url, const  QMap<QString, QString>& headers={}, const QMap<QString, QString>& params = {});
    Response post(const QString &url, const QMap<QString, QString>& headers={}, const QMap<QString, QString>& data={});
private:
    Response request(int type, const std::string &url, const QMap<QString, QString>& headersMap={}, const std::string &data = "");

    std::atomic<bool> *m_isCancelled;
    // CURL* m_curl;



    void setDefaultOpts(CURL* curl);
    static int progress_callback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
        Client* handler = static_cast<Client*>(clientp);
        std::atomic<bool> *shouldCancel = handler->m_isCancelled;
        if (shouldCancel && *shouldCancel) {
            qDebug() << "Request canceled!";
            throw MyException("Request canceled!");
            return 1;
        }
        return 0;
    }
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    static size_t HeaderCallback(char* buffer, size_t size, size_t nitems, void* userdata);
    // inline static QList<CURL*> m_curls;
    // inline static QMutex mutex;
    // inline static bool initialised = false;
};


