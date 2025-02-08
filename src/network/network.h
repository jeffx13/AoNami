#pragma once
#include <QDebug>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutex>
#include <QTimer>
#include "curl/curl.h"
#include "csoup.h"
#include "utils/myexception.h"
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
        std::vector<uint8_t> content;

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
    Client(std::atomic<bool>* shouldCancel, bool verbose = true): m_isCancelled(shouldCancel), m_verbose(verbose) { }

    void setShouldCancel(std::atomic<bool>* shouldCancel) {
        m_isCancelled = shouldCancel;
    }
    bool isOk(const QString& url, const QHash<QString, QString> &headers = {}, long timeout = 5L);
    Response get(const QString &url, const  QMap<QString, QString>& headers={}, const QMap<QString, QString>& params = {}, bool raw = false);
    Response post(const QString &url, const QMap<QString, QString>& data={}, const QMap<QString, QString>& headers={}, bool raw = false);
private:
    Response request(int type, const std::string &url, const QMap<QString, QString>& headersMap={}, const std::string &data = "", bool raw = false);

    std::atomic<bool> *m_isCancelled;
    bool m_verbose;

    void setDefaultOpts(CURL* curl);
    static int progress_callback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
        Client* handler = static_cast<Client*>(clientp);
        std::atomic<bool> *shouldCancel = handler->m_isCancelled;
        if (shouldCancel && *shouldCancel) {
            throw MyException("Request canceled!", "Network");
            return 1;
        }
        return 0;
    }
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp);
    static size_t headerCallback(char* buffer, size_t size, size_t nitems, void* userdata);
    static size_t rawWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        size_t totalBytes(size * nmemb);
        std::vector<uint8_t>* rawBytes = static_cast<std::vector<uint8_t>*>(userp);
        rawBytes->insert(rawBytes->end(), static_cast<uint8_t*>(contents), static_cast<uint8_t*>(contents) + totalBytes);
        return totalBytes;
    }
};


