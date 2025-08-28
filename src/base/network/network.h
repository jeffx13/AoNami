#pragma once
#include <QDebug>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrlQuery>
#include <QString>
#include <QByteArray>
#include <QMap>
#include <QHash>
#include <atomic>
#include <vector>
#include "base/network/csoup.h"
#include "app/appexception.h"
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
        } catch (const AppException &ex) {
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

