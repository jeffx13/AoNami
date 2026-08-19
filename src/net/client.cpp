#include "net/client.h"
#include "net/cloudflare.h"
#include "app/logger.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslError>
#include <QEventLoop>
#include <QTimer>
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

// One QNetworkAccessManager per thread, for connection and SSL session reuse.
static QNetworkAccessManager *getOrCreateNAM() {
    static thread_local QNetworkAccessManager *nam = nullptr;
    if (!nam) {
        nam = new QNetworkAccessManager;
        nam->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
        nam->setCookieJar(new Cloudflare::ProxyCookieJar(nam));
    }
    return nam;
}

static constexpr char k_defaultUserAgent[] =
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
    "(KHTML, like Gecko) Chrome/139.0.0.0 Safari/537.36";

static const char *k_typeNames[] = {"GET", "POST", "HEAD"};

Client::Response Client::request(int type, const QString &urlStr, const QMap<QString, QString> &headersMap, const QByteArray &postData, bool binary) {
    if (urlStr.isEmpty()) return {};

    const QUrl parsedUrl(urlStr);
    const QString host = parsedUrl.host();

    QNetworkAccessManager &manager = *getOrCreateNAM();

    QNetworkRequest request{QUrl(urlStr)};
    request.setTransferTimeout(m_timeoutMs);

    // Don't clobber a provider's UA/Accept with our defaults (ok.ru signs URLs to an exact UA).
    bool hasUserAgent = false, hasAccept = false;
    QString callerCookies;
    for (auto it = headersMap.constBegin(); it != headersMap.constEnd(); ++it) {
        request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
        const QString lower = it.key().toLower();
        if (lower == "user-agent")   hasUserAgent = true;
        else if (lower == "accept")  hasAccept = true;
        else if (lower == "cookie")  callerCookies = it.value();
    }

    // Qt would replace a caller's own header (bilibili's SESSDATA) outright, so merge
    // here and take the jar off the request. Otherwise ProxyCookieJar handles it.
    if (!callerCookies.isEmpty()) {
        const QByteArray merged = Cloudflare::CookieStore::instance().cookieHeader(parsedUrl, callerCookies);
        request.setRawHeader("Cookie", merged);
        request.setAttribute(QNetworkRequest::CookieLoadControlAttribute, QNetworkRequest::Manual);
    }

    if (const QString hostUa = Cloudflare::hostUserAgent(host); !hostUa.isEmpty()) {
        request.setRawHeader("User-Agent", hostUa.toUtf8());
        hasUserAgent = true;
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
        QString msg = QString("%1 (%2)").arg(k_typeNames[type]).arg(response.code);
        if (response.code == 200 || response.code == 206)
            gLog() << msg << urlStr;
        else
            oLog() << msg << urlStr;
    }

    // Qt calls 403/503 an error and drops the payload - which is the only thing that
    // tells a CF interstitial from an origin refusal.
    QMap<QString, QString> replyHeaders;
    for (const QByteArray &header : reply->rawHeaderList())
        replyHeaders[QString::fromUtf8(header)] = QString::fromUtf8(reply->rawHeader(header));
    // A timed-out reply has already closed its device; reading it just warns.
    const QByteArray replyBody = reply->isOpen() ? reply->readAll() : QByteArray();

    const bool failed = reply->error() != QNetworkReply::NoError;
    if (failed && m_verbose) oLog() << "Network" << reply->errorString();
    reply->deleteLater();

    // Clear it and retry once, with the bypass off or a second block loops.
    if (m_bypass && Cloudflare::isBlocked(response.code, replyHeaders,
                                          QString::fromUtf8(replyBody.left(Cloudflare::kBodyScanBytes)))) {
        Cloudflare::markBlocked(host);
        if (!Cloudflare::solveChallenge(parsedUrl, m_cancel).isEmpty()) {
            Client plain = *this;
            plain.m_bypass = false;
            if (Response retry = plain.request(type, urlStr, headersMap, postData, binary); retry.code > 0)
                return retry;
        }
    }

    if (failed) return response;

    response.headers = replyHeaders;
    if (binary) response.bytes = replyBody;
    else        response.body  = QString::fromUtf8(replyBody);

    return response;
}
