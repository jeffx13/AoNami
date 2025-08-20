#include "network.h"
#include "app/myexception.h"
// #include "gui/errordisplayer.h"
#include "app/logger.h"
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
    return request(GET, fullUrl, headers, "");
}

Client::Response Client::post(const QString &url, const QMap<QString, QString> &data, const QMap<QString, QString> &headers){
    QString postData;
    for (auto it = data.constBegin(); it != data.constEnd(); ++it) {
        postData += it.key() + "=" + it.value() + "&";
    }
    return request(POST, url, headers, postData.toUtf8());
}

Client::Response Client::request(int type, const QString &urlStr, const QMap<QString, QString> &headersMap, const QByteArray &postData) {
    if (urlStr.isEmpty()) return Response(400);

    QUrl url(urlStr);
    QNetworkRequest request(url);
    // Set headers
    for (auto it = headersMap.begin(); it != headersMap.end(); ++it) {
        request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
    }
    QNetworkAccessManager manager;
    manager.setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
    // manager.setTransferTimeout(10000);
    QNetworkReply* reply = nullptr;
    Client::Response response;


    switch (type) {
    case GET:
        reply = manager.get(request);
        break;
    case POST:
        reply = manager.post(request, postData);
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
        QTimer *cancelTimer = new QTimer(&manager);
        cancelTimer->setInterval(100);
        QObject::connect(cancelTimer, &QTimer::timeout, reply, [reply, this, urlStr, cancelTimer](){
            if (isCancelled()) {
                reply->abort();
                cancelTimer->stop();
                cancelTimer->deleteLater();
            }
        }, Qt::QueuedConnection);
        cancelTimer->start();
    }

    loop.exec();

    if (isCancelled()) {
        reply->deleteLater();
        // mLog() << "Request" << "Cancelled:" << urlStr;
        return response;
    }

    QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    if (statusCode.isValid()) {
        response.code = statusCode.toInt();
    }

    if (m_verbose) {
        QString typeStr = type == GET ? "GET" : type == POST ? "POST" : type == HEAD ? "HEAD" : "UNKNOWN";
        if (response.code == 200) {
            gLog() << QString("%1 (%2)").arg(typeStr).arg(response.code) << urlStr;
        } else {
            oLog() << QString("%1 (%2)").arg(typeStr).arg(response.code) << urlStr;
        }
    }

    if (reply->error() != QNetworkReply::NoError) {
        reply->deleteLater();
        return response;
    }

    const QList<QByteArray> headerList = reply->rawHeaderList();
    Q_FOREACH(const QByteArray &header, headerList) {
        response.headers[QString(header)] = reply->rawHeader(header);
    }

    QByteArray body = reply->readAll();
    response.body = body;
    reply->deleteLater();

    return response;
}
