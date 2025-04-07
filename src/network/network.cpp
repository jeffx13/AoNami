#include "network.h"
#include "utils/myexception.h"
// #include "utils/errorhandler.h"
#include "utils/logger.h"
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>

Client::Response Client::get(const QString &url, const QMap<QString, QString> &headers, const QMap<QString, QString> &params) {
    auto fullUrl = url;
    if (!params.isEmpty()) {
        fullUrl += "?";
        for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
            fullUrl += it.key() + "=" + it.value() + "&";
        }
        fullUrl.chop(1);
    }
    auto urlString = url.toStdString();
    // return request(GET, fullUrl.toStdString(), headers, "", raw);
    return request(GET, fullUrl, headers, "");
}

Client::Response Client::post(const QString &url, const QMap<QString, QString> &data, const QMap<QString, QString> &headers){
    QString postData;
    for (auto it = data.constBegin(); it != data.constEnd(); ++it) {
        postData += it.key() + "=" + it.value() + "&";
    }
    // return request(POST, url.toStdString(), headers, postData.toStdString(), raw);
    return request(POST, url, headers, postData);

}

Client::Response Client::request(int type, const QString &urlStr, const QMap<QString, QString> &headersMap, const QString &postData) {
    QUrl url(urlStr);
    QNetworkRequest request(url);
    // Set headers
    for (auto it = headersMap.begin(); it != headersMap.end(); ++it) {
        request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
    }
    QNetworkAccessManager manager;
    QNetworkReply* reply = nullptr;
    Client::Response response;

    QByteArray postBytes = postData.toUtf8();

    switch (type) {
    case GET:
        reply = manager.get(request);
        break;
    case POST:
        reply = manager.post(request, postBytes);
        break;
    case HEAD:
        request.setAttribute(QNetworkRequest::CustomVerbAttribute, "HEAD");
        reply = manager.sendCustomRequest(request, "HEAD");
        break;
    default:
        throw MyException("Unsupported request type", "Network");
    }


    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    if (m_isCancelled) {
        QObject::connect(reply, &QNetworkReply::downloadProgress, [reply, response, this, urlStr](qint64 byteReceived, qint64 byteTotal) {
            if (response.code == -1 && isCancelled()) {
                reply->abort();
                mLog() << "Request cancelled" << urlStr;
            }
            //mLog() << "Download progress" << byteReceived << "/" << byteTotal;

        });

    }
    loop.exec();

    if (isCancelled()) {
        reply->deleteLater();
        qDebug() << "Network" << "Cancelled: " << urlStr;;
        return response;
    }

    if (reply->error() != QNetworkReply::NoError) {
        QString errorMessage = QString("%1 : %2").arg(urlStr, reply->errorString());
        reply->deleteLater();
        throw MyException(errorMessage, QString("%1 X").arg(type == GET ? "GET" : "POST"));
    }

    // Response code
    QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    if (statusCode.isValid()) {
        response.code = statusCode.toInt();
    }

    // Headers
    const QList<QByteArray> headerList = reply->rawHeaderList();
    for (const QByteArray &header : headerList) {
        response.headers[QString(header)] = reply->rawHeader(header);
    }

    // Body
    QByteArray body = reply->readAll();
    response.body = body;

    reply->deleteLater();

    if (m_verbose) {
        QString typeStr = type == GET ? "GET" : type == POST ? "POST" : type == HEAD ? "HEAD" : "UNKNOWN";
        mLog() << QString("%1 {%2}").arg(typeStr).arg(response.code) << url;
    }

    return response;
}



// void Client::setDefaultOpts(CURL *curl) {
//     if (curl) {
//         curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);
//         curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
//         curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
//         // curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Linux; Android 8.0.0; moto g(6) play Build/OPP27.91-87) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/69.0.3497.100 Mobile Safari/537.36");
//         // curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/131.0.0.0 Safari/537.36");
//         curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
//         curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 1L);
//         // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
//         curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

//         // curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
//         curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
//         curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
//         curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
//         curl_easy_setopt(curl, CURLOPT_XFERINFODATA, this);
//         curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "br, gzip, deflate");
//     }
// }

// int Client::progress_callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
//     Client* handler = static_cast<Client*>(clientp);
//     std::atomic<bool> *shouldCancel = handler->m_isCancelled;
//     if (shouldCancel && *shouldCancel) {
//         throw MyException("Request canceled!", "Network");
//         return 1;
//     }
//     return 0;
// }

// size_t Client::writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
//     size_t totalBytes(size * nmemb);
//     QString* str = static_cast<QString*>(userp);
//     QString newData = QString::fromUtf8(static_cast<char*>(contents), static_cast<int>(totalBytes));
//     str->append(newData);
//     return totalBytes;
// }

// size_t Client::headerCallback(char *buffer, size_t size, size_t nitems, void *userdata) {
//     size_t numbytes = size * nitems;
//     QString* header = static_cast<QString*>(userdata);
//     QString newData = QString::fromUtf8(buffer, static_cast<int>(numbytes));
//     header->append(newData);
//     return numbytes;
// }

// size_t Client::rawWriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
//     size_t totalBytes(size * nmemb);
//     std::vector<uint8_t>* rawBytes = static_cast<std::vector<uint8_t>*>(userp);
//     rawBytes->insert(rawBytes->end(), static_cast<uint8_t*>(contents), static_cast<uint8_t*>(contents) + totalBytes);
//     return totalBytes;
// }

// Client::Response Client::request(int type, const std::string &url, const QMap<QString, QString> &headersMap, const std::string &postData, bool raw){
//     auto m_curl = curl_easy_init();
//     if (!m_curl) {
//         throw MyException("Failed to get curl", "Network");
//     }
//     setDefaultOpts(m_curl);


//     if (m_isCancelled) {
//         if (*m_isCancelled)
//             throw MyException("Request canceled!", "Network");
//         curl_easy_setopt(m_curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
//         curl_easy_setopt(m_curl, CURLOPT_XFERINFODATA, this);
//         curl_easy_setopt(m_curl, CURLOPT_NOPROGRESS, 0L);
//     }


//     Response response;

//     curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());

//     struct curl_slist* curlHeaders = NULL;
//     if (!headersMap.isEmpty()) {
//         for(auto it = headersMap.begin(); it != headersMap.end(); ++it) {
//             std::string header = it.key().toStdString() + ": " + it.value().toStdString();
//             curlHeaders = curl_slist_append(curlHeaders, header.c_str());
//         }
//         curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, curlHeaders);
//     }


//     switch (type) {
//     case POST:
//         curl_easy_setopt(m_curl, CURLOPT_POST, 1L);
//         curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, postData.c_str());
//         break;
//     case HEAD:
//         curl_easy_setopt(m_curl, CURLOPT_NOBODY, 1L);  // <- HEAD request
//         break;
//     }
//     curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, &headerCallback);
//     curl_easy_setopt(m_curl, CURLOPT_HEADERDATA, &response.headers);

//     if (type != HEAD) {
//         if (raw) {
//             curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &response.content);
//             curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, &rawWriteCallback);
//         } else {
//             curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, &writeCallback);
//             curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &response.body);
//         }
//     }


//     CURLcode res = curl_easy_perform(m_curl);
//     if (res != CURLE_OK){
//         auto errorMessage = QString("%1 : %2").arg(QString::fromStdString(url), QString(curl_easy_strerror(res)));
//         throw MyException(errorMessage, QString("%1 X").arg(type == GET ? "GET" : "POST"));
//     }

//     curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &response.code);
//     if (curlHeaders)
//         curl_slist_free_all(curlHeaders);

//     curl_easy_cleanup(m_curl);

//     if (m_verbose) {
//         auto typeString = type == GET ? "GET" : type == POST ? "POST" : type == HEAD ? "HEAD" : "UNKNOWN";
//         mLog() << QString("%1 (%2)").arg(typeString).arg(response.code) << url;
//     }

//     return response;
// }




