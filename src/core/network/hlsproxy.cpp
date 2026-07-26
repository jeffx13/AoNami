#include "hlsproxy.h"
#include <QTcpSocket>
#include <QThreadPool>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QUrl>
#include <QUrlQuery>
#include <QTimer>

HlsProxy *HlsProxy::s_instance = nullptr;

static const QByteArray kUserAgent =
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:143.0) Gecko/20100101 Firefox/143.0";

HlsProxy::HlsProxy(QObject *parent) : QTcpServer(parent) {
    s_instance = this;
    ensureListening();
}

void HlsProxy::ensureListening() {
    if (m_port.load() != 0) return;
    if (listen(QHostAddress::LocalHost, 0))
        m_port.store(serverPort());
}

static QString pct(const QString &s) { return QString::fromUtf8(QUrl::toPercentEncoding(s)); }

static QString proxyUrl(quint16 port, const QString &endpoint, const QString &url, const QString &ref) {
    return QString("http://127.0.0.1:%1/%2?u=%3&r=%4").arg(port).arg(endpoint).arg(pct(url), pct(ref));
}

QString HlsProxy::playlistUrl(const QString &m3u8Url, const QString &referer) {
    quint16 p = m_port.load();
    if (p == 0) return m3u8Url;
    return proxyUrl(p, "p.m3u8", m3u8Url, referer);
}

// Rewrite a playlist so every sub-playlist / segment url points back through the proxy (carrying the
// same upstream Referer). Sub-playlists keep going through /p.m3u8; media segments hit /s.ts.
static QString rewritePlaylist(const QString &content, const QString &base, const QString &ref, quint16 port) {
    QStringList out;
    for (const QString &line : content.split('\n')) {
        QString t = line.trimmed();
        if (t.isEmpty()) continue;
        if (t.startsWith('#')) { out << t; continue; }
        QString abs = t.startsWith("http") ? t : QUrl(base).resolved(QUrl(t)).toString();
        bool isPl = abs.split('?').first().endsWith(".m3u8");
        out << proxyUrl(port, isPl ? "p.m3u8" : "s.ts", abs, ref);
    }
    return out.join('\n');
}

// Offset of the first MPEG-TS packet (0x47 repeating every 188 bytes), i.e. past any junk prefix.
static int tsOffset(const QByteArray &d) {
    const int lim = std::min<int>(int(d.size()) - 188 * 3, 8192);
    for (int o = 0; o < lim; ++o) {
        if (quint8(d[o]) == 0x47 && quint8(d[o + 188]) == 0x47 &&
            quint8(d[o + 376]) == 0x47 && quint8(d[o + 564]) == 0x47)
            return o;
    }
    return 0;
}

static QByteArray fetch(const QString &url, const QString &referer) {
    QNetworkAccessManager nam;
    QNetworkRequest r{QUrl(url)};
    r.setRawHeader("User-Agent", kUserAgent);
    if (!referer.isEmpty()) r.setRawHeader("Referer", referer.toUtf8());
    r.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    QNetworkReply *reply = nam.get(r);
    QEventLoop loop;
    QTimer::singleShot(20000, &loop, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    QByteArray data = reply->isFinished() ? reply->readAll() : QByteArray();
    reply->deleteLater();
    return data;
}

void HlsProxy::incomingConnection(qintptr handle) {
    const quint16 port = m_port.load();
    QThreadPool::globalInstance()->start([handle, port]() {
        QTcpSocket sock;
        if (!sock.setSocketDescriptor(handle)) return;

        QByteArray req;
        while (!req.contains("\r\n\r\n") && sock.waitForReadyRead(4000))
            req += sock.readAll();
        const QList<QByteArray> parts = req.left(req.indexOf("\r\n")).split(' ');
        if (parts.size() < 2) return;

        const QUrl target(QString::fromUtf8(parts[1]));
        const QUrlQuery q(target.query());
        const QString url = q.queryItemValue("u", QUrl::FullyDecoded);
        const QString ref = q.queryItemValue("r", QUrl::FullyDecoded);

        QByteArray body, contentType;
        if (!url.isEmpty()) {
            const QByteArray data = fetch(url, ref);
            if (target.path().endsWith(".m3u8")) {
                body = rewritePlaylist(QString::fromUtf8(data), url, ref, port).toUtf8();
                contentType = "application/vnd.apple.mpegurl";
            } else {
                body = data.isEmpty() ? data : data.mid(tsOffset(data));
                contentType = "video/mp2t";
            }
        }

        QByteArray head = "HTTP/1.1 200 OK\r\n"
                          "Content-Type: " + contentType + "\r\n"
                          "Access-Control-Allow-Origin: *\r\n"
                          "Content-Length: " + QByteArray::number(body.size()) + "\r\n"
                          "Connection: close\r\n\r\n";
        // mpv often aborts segment requests mid-flight, so bail the moment the socket is gone.
        if (sock.state() != QAbstractSocket::ConnectedState) return;
        sock.write(head);
        sock.write(body);
        while (sock.bytesToWrite() > 0 && sock.state() == QAbstractSocket::ConnectedState)
            if (!sock.waitForBytesWritten(10000)) break;
        if (sock.state() == QAbstractSocket::ConnectedState) {
            sock.disconnectFromHost();
            if (sock.state() != QAbstractSocket::UnconnectedState)
                sock.waitForDisconnected(3000);
        }
    });
}
