#include "network.h"
#include "app/logger.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslError>
#include <QEventLoop>
#include <QTimer>
#include <QThread>
#include <QUrl>
#include <QUrlQuery>

static QString buildUrl(const QString &url, const QMap<QString, QString> &params) {
    QUrl fullUrl(url);
    if (!params.isEmpty()) {
        QUrlQuery query;
        for (auto it = params.constBegin(); it != params.constEnd(); ++it)
            query.addQueryItem(it.key(), it.value());
        fullUrl.setQuery(query);
    }
    return fullUrl.toString(QUrl::FullyEncoded);
}

Client::Response Client::get(const QString &url, const QMap<QString, QString> &headers, const QMap<QString, QString> &params) {
    return request(GET, buildUrl(url, params), headers, {});
}

Client::Response Client::getBytes(const QString &url, const QMap<QString, QString> &headers, const QMap<QString, QString> &params) {
    return request(GET, buildUrl(url, params), headers, {}, true);
}

Client::Response Client::post(const QString &url, const QMap<QString, QString> &data, const QMap<QString, QString> &headers) {
    QUrlQuery query;
    for (auto it = data.constBegin(); it != data.constEnd(); ++it)
        query.addQueryItem(it.key(), it.value());
    return request(POST, url, headers, query.query(QUrl::FullyEncoded).toUtf8());
}

// One QNetworkAccessManager per thread for connection/SSL session reuse.

static QNetworkAccessManager *getOrCreateNAM() {
    static thread_local QNetworkAccessManager *nam = nullptr;
    if (!nam) {
        nam = new QNetworkAccessManager;
        nam->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
    }
    return nam;
}

static constexpr char k_defaultUserAgent[] =
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
    "(KHTML, like Gecko) Chrome/139.0.0.0 Safari/537.36";

Client::Response Client::request(int type, const QString &urlStr, const QMap<QString, QString> &headersMap, const QByteArray &postData, bool binary) {
    if (urlStr.isEmpty()) return {};

    QNetworkAccessManager &manager = *getOrCreateNAM();

    QNetworkRequest request{QUrl(urlStr)};
    request.setTransferTimeout(10000);

    // Don't clobber a provider's UA/Accept with our defaults (ok.ru signs URLs to an exact UA).
    bool hasUserAgent = false, hasAccept = false;
    for (auto it = headersMap.constBegin(); it != headersMap.constEnd(); ++it) {
        request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
        const QString lower = it.key().toLower();
        if (lower == "user-agent") hasUserAgent = true;
        else if (lower == "accept") hasAccept = true;
    }

    if (!hasUserAgent)
        request.setRawHeader("User-Agent", k_defaultUserAgent);
    if (!hasAccept)
        request.setRawHeader("Accept", "*/*");

    QNetworkReply *reply = nullptr;
    switch (type) {
    case GET:  reply = manager.get(request); break;
    case POST: reply = manager.post(request, postData); break;
    case HEAD: reply = manager.head(request); break;
    default:   return {};
    }

    // Some CDNs serve valid streams behind certs Qt distrusts; mpv plays them - don't reject on SSL.
    QObject::connect(reply, &QNetworkReply::sslErrors, reply,
                     [reply](const QList<QSslError> &) { reply->ignoreSslErrors(); });

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    QTimer cancelTimer;
    cancelTimer.setInterval(50);
    QObject::connect(&cancelTimer, &QTimer::timeout, reply, [reply, this]() {
        if (isCancelled()) reply->abort();
    });
    QObject::connect(reply, &QNetworkReply::finished, &cancelTimer, &QTimer::stop);
    cancelTimer.start();
    loop.exec();

    Response response;
    if (isCancelled()) { reply->deleteLater(); return response; }

    QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    if (statusCode.isValid()) response.code = statusCode.toInt();

    if (m_verbose) {
        static const char *typeNames[] = {"GET", "POST", "HEAD"};
        QString msg = QString("%1 (%2)").arg(typeNames[type]).arg(response.code);
        if (response.code == 200 || response.code == 206)
            gLog() << msg << urlStr;
        else
            oLog() << msg << urlStr;
    }

    if (reply->error() != QNetworkReply::NoError) {
        if (m_verbose) oLog() << "Network" << reply->errorString();
        reply->deleteLater();
        return response;
    }

    for (const QByteArray &header : reply->rawHeaderList())
        response.headers[QString::fromUtf8(header)] = QString::fromUtf8(reply->rawHeader(header));

    if (binary) response.bytes = reply->readAll();
    else        response.body  = QString::fromUtf8(reply->readAll());

    reply->deleteLater();
    return response;
}