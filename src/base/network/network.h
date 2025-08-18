#pragma once
#include <QDebug>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutex>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include "base/network/csoup.h"
#include "app/myexception.h"
#include <QJsonArray>

class Client {
public:
    class Response;
    class Request;
    Client(std::atomic<bool>* shouldCancel = nullptr, bool verbose = true): m_isCancelled(shouldCancel), m_verbose(verbose) { }
    Client(const Client &other) : m_isCancelled(other.m_isCancelled), m_verbose(other.m_verbose) {}
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
    inline Response post(const QString &url, const QByteArray& data={}, const QMap<QString, QString>& headers={}) {
        return request(POST, url, headers, data);
    }


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

    // class Request {

    //     Request(const QString &url, RequestType type = RequestType::GET): request(url){ }
    //     Request(const QUrl &url, RequestType type = RequestType::GET) : request(url) { }

    //     Request &setUrl(const QUrl &url) {
    //         request.setUrl(url);
    //         return *this;
    //     }

    //     Request &setUrl(const QString &url) {
    //         request.setUrl(url);
    //         return *this;
    //     }

    //     Request &setType(const QString &url) {
    //         m_url = QUrl::fromUserInput(url);
    //         return *this;
    //     }

    //     Request &setHeaders(const QMap<QString, QString> &headers) {
    //         for (auto it = headers.begin(); it != headers.end(); ++it) {
    //             request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
    //         }
    //         m_headers = headers;
    //         return *this;
    //     }

    //     Request &setData(const QMap<QString, QString> &data) {
    //         QString dataStr = "";
    //         for (auto it = data.constBegin(); it != data.constEnd(); ++it) {
    //             dataStr += it.key() + "=" + it.value() + "&";
    //         }
    //         m_data = dataStr.toUtf8();
    //         return *this;
    //     }


    // private:
    //     QNetworkRequest request;
    //     QUrl m_url;
    //     Client::RequestType m_type;
    //     QByteArray m_data;
    //     QMap<QString, QString> m_headers;
    // };

    struct Response {
        long code = -1;
        QString redirectUrl;
        QMap<QString, QString> headers;
        QString body;
        QMap<QString, QString> cookies;
        std::vector<uint8_t> content;

        QJsonObject toJsonObject(){
            if (body.isEmpty()) return QJsonObject();

            QJsonParseError error;
            QJsonDocument jsonData = QJsonDocument::fromJson(body.toUtf8(), &error);
            if (error.error != QJsonParseError::NoError) {
                oLog() << "JSON" << error.errorString() << ": " << body;
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
    Response request(int type, const QString &urlStr, const QMap<QString, QString> &headersMap, const QByteArray &postData = {});

};

