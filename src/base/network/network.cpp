#include "network.h"
#include "app/appexception.h"
#include "app/logger.h"
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QEventLoop>
#include <QTimer>
#include <QUrl>
#include <QVariant>
#include <QUrlQuery>
#include <QString>
#include <QHash>

Client::Response Client::get(const QString &url, const QMap<QString, QString> &headers, const QMap<QString, QString> &params) {
    QUrl fullUrl(url);
    if (!params.isEmpty()) {
        QUrlQuery query;
        for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
            query.addQueryItem(it.key(), it.value());
        }
        fullUrl.setQuery(query);
    }
    return request(GET, fullUrl.toString(QUrl::FullyEncoded), headers, QByteArray());
}

Client::Response Client::post(const QString &url, const QMap<QString, QString> &data, const QMap<QString, QString> &headers) {
    QUrlQuery query;
    for (auto it = data.constBegin(); it != data.constEnd(); ++it) {
        query.addQueryItem(it.key(), it.value());
    }
    QByteArray postData = query.query(QUrl::FullyEncoded).toUtf8();
    return request(POST, url, headers, postData);
}

Client::Response Client::request(int type, const QString &urlStr, const QMap<QString, QString> &headersMap, const QByteArray &postData) {
    if (urlStr.isEmpty()) return Response();

    QUrl url(urlStr);
    QNetworkRequest request(url);

    // Set headers
    for (auto it = headersMap.constBegin(); it != headersMap.constEnd(); ++it) {
        request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
    }
    // Reasonable defaults if caller did not provide
    if (!headersMap.contains("User-Agent")) {
        request.setRawHeader("User-Agent", QByteArray("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/139.0.0.0 Safari/537.36"));
    }
    if (!headersMap.contains("Accept")) {
        request.setRawHeader("Accept", QByteArray("*/*"));
    }

    QNetworkAccessManager manager;
    manager.setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);

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
        throw AppException("Unsupported request type", "Network");
    }

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    QTimer cancelTimer;
    if (m_isCancelled) {
        cancelTimer.setInterval(100);
        QObject::connect(&cancelTimer, &QTimer::timeout, reply, [reply, this, &cancelTimer]() {
            if (isCancelled()) {
                reply->abort();
                cancelTimer.stop();
            }
        }, Qt::QueuedConnection);
        cancelTimer.start();
    }

    loop.exec();

    if (isCancelled()) {
        reply->deleteLater();
        return response;
    }

    // Handle redirects
    QVariant redirectTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (redirectTarget.isValid()) {
        QUrl redirectUrl = QUrl(url).resolved(redirectTarget.toUrl());
        response.redirectUrl = redirectUrl.toString();
    }

    QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    if (statusCode.isValid()) {
        response.code = statusCode.toInt();
    }

    if (m_verbose) {
        QString typeStr;
        switch (type) {
            case GET: typeStr = "GET"; break;
            case POST: typeStr = "POST"; break;
            case HEAD: typeStr = "HEAD"; break;
            default: typeStr = "UNKNOWN"; break;
        }
        if (response.code == 200) {
            gLog() << QString("%1 (%2)").arg(typeStr).arg(response.code) << urlStr;
        } else {
            oLog() << QString("%1 (%2)").arg(typeStr).arg(response.code) << urlStr;
        }
    }

    if (reply->error() != QNetworkReply::NoError) {
        if (m_verbose) {
            oLog() << "Network" << reply->errorString();
        }
        reply->deleteLater();
        return response;
    }

    const QList<QByteArray> headerList = reply->rawHeaderList();
    for (const QByteArray &header : headerList) {
        response.headers[QString::fromUtf8(header)] = QString::fromUtf8(reply->rawHeader(header));
    }

    QByteArray body = reply->readAll();
    response.body = QString::fromUtf8(body);
    response.content.assign(reinterpret_cast<const uint8_t*>(body.constData()), reinterpret_cast<const uint8_t*>(body.constData()) + body.size());
    reply->deleteLater();

    return response;
}
