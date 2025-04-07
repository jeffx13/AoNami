#pragma once
#include <QDebug>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutex>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
// #include "curl/curl.h"
#include "csoup.h"
#include "utils/myexception.h"
#include <QJsonArray>

class Client {
public:
    class Response;
    Client(std::atomic<bool>* shouldCancel, bool verbose = true): m_isCancelled(shouldCancel), m_verbose(verbose) { }
    // Copy constructor
    Client(const Client &other) : m_isCancelled(other.m_isCancelled), m_verbose(other.m_verbose) {}
    // equal operator
    Client& operator=(const Client &other) {
        if (this != &other) {
            m_isCancelled = other.m_isCancelled;
            m_verbose = other.m_verbose;
        }
        return *this;
    }


    void setShouldCancel(std::atomic<bool>* shouldCancel) {
        m_isCancelled = shouldCancel;
    }
    bool isCancelled() {
        if (m_isCancelled) return m_isCancelled->load();
        return false;
    }

    bool isOk(const QString& url, const QHash<QString, QString> &headers = {}, long timeout = 5L);
    Response get(const QString &url, const  QMap<QString, QString>& headers={}, const QMap<QString, QString>& params = {});
    Response post(const QString &url, const QMap<QString, QString>& data={}, const QMap<QString, QString>& headers={});
    Response head(const QString &url, const QMap<QString, QString> &headers) {
        return request(HEAD, url, headers);
    }

    int partialGet(const QString &url, const QMap<QString, QString> &headers = {}, const QString &range = "0-0") {
        auto rangeHeaders = headers;
        rangeHeaders.insert("Range", "bytes=" + range);
        try {
            auto response = get(url, rangeHeaders);
            return (response.code == 206 || response.code == 200);
        } catch (const MyException &ex) {
            return false;
        }
    }

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
        long code = -1;
        // QUrl url;
        QString redirectUrl;
        QMap<QString, QString> headers;
        QString body;
        QMap<QString, QString> cookies;
        std::vector<uint8_t> content;

        QJsonObject toJsonObject(){
            QJsonParseError error;
            QJsonDocument jsonData = QJsonDocument::fromJson(body.toUtf8(), &error);
            if (error.error != QJsonParseError::NoError) {
                oLog() << "JSON parsing error" << error.errorString();
                return QJsonObject();
            }
            return jsonData.object();
        }
        QJsonArray toJsonArray(){
            QJsonParseError error;
            QJsonDocument jsonData = QJsonDocument::fromJson(body.toUtf8(), &error);
            if (error.error != QJsonParseError::NoError) {
                oLog() << "JSON parsing error" << error.errorString();
                return QJsonArray();
            }
            return jsonData.array();
        }
        CSoup toSoup(){
            return CSoup::parse(body);
        }

        ~Response(){}
    };


private:
    std::atomic<bool> *m_isCancelled;
    bool m_verbose;
    Response request(int type, const QString &urlStr, const QMap<QString, QString> &headersMap, const QString &postData = "");

};

// void setDefaultOpts(CURL* curl);
// static int progress_callback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);
// static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp);
// static size_t headerCallback(char* buffer, size_t size, size_t nitems, void* userdata);
// static size_t rawWriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
// Response request(int type, const std::string &url, const QMap<QString, QString>& headersMap={}, const std::string &data = "");



