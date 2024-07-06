#pragma once
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutex>
#include "curl/curl.h"
#include <QJsonArray>

class NetworkClient
{
private:
    static CURL* getCurl(){
        QMutexLocker locker(&mutex);
        CURL* curl = curls.back();
        initCurl(curl);
        curls.pop_back();
        return curl;
    }

    static void returnCurl(CURL* curl){
        if (!curl) return;
        QMutexLocker locker(&mutex);
        curl_easy_reset (curl);
        curls.push_back(curl);
    }
    static void initCurl(CURL* curl) {
        if (curl) {
            // Set the timeouts
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);

            curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);

            // Set the user agent
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");

            // Set SSL options
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
            //curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &WriteCallback);
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
            curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
        }
    }
    enum RequestType{
        GET,
        POST,
        HEAD,
        PUT,
        DELETE_,
        CONNECT,
        OPTIONS
    };
public:
    static void init(int maxCurls = 5);

    static void cleanUp();

public:
    struct Response{
        long code;
        // QUrl url;
        QString redirectUrl;
        //        QMap<QString, QString> headers;
        QString headers;
        QString body;
        QMap<QString, QString> cookies;

        QJsonObject toJson(){
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

        ~Response(){}
    };

    static bool isUrlValid(const QString& url);

    static Response get(const QString &url, const  QMap<QString, QString>& headers={}, const QMap<QString, QString>& params = {});

    static Response post(const QString &url, const QMap<QString, QString>& headers={}, const QMap<QString, QString>& data={});
private:
    static Response request(int type, const std::string &url, const QMap<QString, QString>& headersMap={}, const std::string &data = "");

private:
    NetworkClient() = default;
    ~NetworkClient() = default;

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    static size_t HeaderCallback(char* buffer, size_t size, size_t nitems, void* userdata);

private:
    inline static QList<CURL*> curls;
    inline static QMutex mutex;
    inline static bool initialised = false;

};


