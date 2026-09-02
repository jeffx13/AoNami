#include <winsock2.h>   // before anything that might pull in windows.h
#include "net/hlsproxy.h"
#include "net/cloudflare.h"
#include "net/client.h"
#include <QCoreApplication>
#include <QTcpSocket>
#include <QUrl>
#include <QUrlQuery>
#include <QDateTime>
#include <QHash>
#include <QElapsedTimer>
#include <QRandomGenerator>
#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <QRegularExpression>

HlsProxy *HlsProxy::s_instance = nullptr;

// Without this a fetch on a slow upstream keeps waitForDone(), and the app, alive.
static CancelToken g_proxyCancel;

static const QByteArray kUserAgent = kFirefoxUserAgent;

HlsProxy::HlsProxy(QObject *parent) : QTcpServer(parent) {
    s_instance = this;
    g_proxyCancel.reset();
    m_secret.resize(32);
    auto *words = reinterpret_cast<quint32 *>(m_secret.data());
    QRandomGenerator::system()->generate(words, words + 8);
    // Also sits in front of server verification, which probes many servers at once.
    m_pool.setMaxThreadCount(16);
    m_pool.setExpiryTimeout(30000);
    ensureListening();

    // The destructor runs too late to stop a fetch already on a slow upstream.
    QObject::connect(qApp, &QCoreApplication::aboutToQuit, this, [this] {
        close();
        g_proxyCancel.cancel();
    });
}

HlsProxy::~HlsProxy() {
    close();                  // stop accepting before the pool drains
    g_proxyCancel.cancel();
    m_pool.waitForDone();
    s_instance = nullptr;
}

void HlsProxy::ensureListening() {
    if (m_port.load() != 0) return;
    if (listen(QHostAddress::LocalHost, 0))
        m_port.store(serverPort());
}

static QString pct(const QString &s) { return QString::fromUtf8(QUrl::toPercentEncoding(s)); }

// Anything on loopback can reach this port, so only serve urls the proxy itself minted.
static QByteArray sign(const QByteArray &secret, const QString &url, const QString &ref) {
    QMessageAuthenticationCode mac(QCryptographicHash::Sha256, secret);
    mac.addData(url.toUtf8());
    mac.addData("\x1f");   // field separator: the (u, r) boundary can't be shifted
    mac.addData(ref.toUtf8());
    return mac.result().left(16).toHex();
}

static QString proxyUrl(quint16 port, const QByteArray &secret, const char *endpoint,
                        const QString &url, const QString &ref) {
    return QString("http://127.0.0.1:%1/%2?u=%3&r=%4&s=%5")
        .arg(port).arg(QLatin1String(endpoint), pct(url), pct(ref),
                       QString::fromLatin1(sign(secret, url, ref)));
}

QString HlsProxy::playlistUrl(const QString &m3u8Url, const QString &referer) {
    quint16 p = m_port.load();
    if (p == 0) return m3u8Url;
    return proxyUrl(p, m_secret, "p.m3u8", m3u8Url, referer);
}

static QString rewritePlaylist(const QString &content, const QString &base, const QString &ref,
                               quint16 port, const QByteArray &secret) {
    static const QRegularExpression uriRe(QStringLiteral(R"RX(URI="([^"]+)")RX"));
    const QUrl baseUrl(base);
    QStringList out;
    for (const QString &line : content.split('\n')) {
        QString t = line.trimmed();
        if (t.isEmpty()) continue;
        if (t.startsWith('#')) {
            // Keys and init segments hide in attributes and 403 without our Referer.
            if (t.startsWith("#EXT-X-KEY") || t.startsWith("#EXT-X-SESSION-KEY") || t.startsWith("#EXT-X-MAP")) {
                const auto m = uriRe.match(t);
                if (m.hasMatch())
                    t.replace(m.capturedStart(1), m.capturedLength(1),
                              proxyUrl(port, secret, "k.bin",
                                       baseUrl.resolved(QUrl(m.captured(1))).toString(), ref));
            }
            out << t;
            continue;
        }
        const QString abs = baseUrl.resolved(QUrl(t)).toString();
        const QString path = abs.split('?').first();
        const char *endpoint = path.endsWith(".m3u8")                            ? "p.m3u8"
                             : (path.endsWith(".m4s") || path.endsWith(".mp4"))  ? "k.bin"
                                                                                 : "s.ts";
        out << proxyUrl(port, secret, endpoint, abs, ref);
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

struct Fetched {
    int code = 0;
    QByteArray data;
    QByteArray contentRange;
};

// Client, not a bare QNAM: it brings the CF bypass and jar, and `range` must pass through.
static Fetched fetch(const QString &url, const QString &referer,
                     const QByteArray &range = {}, int timeoutMs = 0) {
    QMap<QString, QString> headers{{QStringLiteral("User-Agent"), QString::fromLatin1(kUserAgent)}};
    if (!referer.isEmpty()) headers[QStringLiteral("Referer")] = referer;
    if (!range.isEmpty())   headers[QStringLiteral("Range")] = QString::fromLatin1(range);
    Cloudflare::applyClearanceHeaders(QUrl(url), headers);

    Client client(g_proxyCancel, false);   // quiet: a line per segment would drown the log
    if (timeoutMs > 0) client.setTimeout(timeoutMs);
    const Client::Response response = client.getBytes(url, headers);

    Fetched f;
    f.code = response.code;
    f.data = response.bytes;
    f.contentRange = response.header("Content-Range").toLatin1();
    return f;
}

// Fetched twice in quick succession: once to verify the server, once when mpv opens it.
static Fetched fetchPlaylist(const QString &url, const QString &referer) {
    // Some hosts build the playlist on demand, going quiet for longer than the default timeout allows.
    constexpr int kBuildTimeoutMs = 45000;
    struct Entry { QByteArray data; int code; qint64 at; };
    static QMutex mutex;
    static QHash<QString, Entry> cache;
    constexpr qint64 kTtlMs = 90000;

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    {
        QMutexLocker lock(&mutex);
        const auto it = cache.constFind(url);
        if (it != cache.constEnd() && now - it->at < kTtlMs) return {it->code, it->data};
    }

    const Fetched f = fetch(url, referer, {}, kBuildTimeoutMs);
    // Finished playlists only - a live one has no ENDLIST and must stay fresh.
    const bool cacheable = f.code >= 200 && f.code < 400 && !f.data.isEmpty()
                           && (f.data.contains("#EXT-X-ENDLIST") || f.data.contains("#EXT-X-STREAM-INF"));
    if (cacheable) {
        QMutexLocker lock(&mutex);
        if (cache.size() > 32) cache.clear();
        cache.insert(url, {f.data, f.code, now});
    }
    return f;
}

static void reply(QTcpSocket &sock, int code, const QByteArray &ctype,
                  const QByteArray &body, bool headOnly = false,
                  const QByteArray &contentRange = {}) {
    const char *phrase = code == 200 ? "OK"
                       : code == 206 ? "Partial Content"
                       : code == 400 ? "Bad Request"
                       : code == 403 ? "Forbidden"
                       : code == 404 ? "Not Found"
                       : code == 405 ? "Method Not Allowed"
                       : code == 502 ? "Bad Gateway" : "Error";
    QByteArray head = "HTTP/1.1 " + QByteArray::number(code) + " " + phrase + "\r\n";
    if (!ctype.isEmpty())        head += "Content-Type: " + ctype + "\r\n";
    if (!contentRange.isEmpty()) head += "Content-Range: " + contentRange + "\r\n";
    head += "Accept-Ranges: bytes\r\n";
    head += "Content-Length: " + QByteArray::number(headOnly ? 0 : body.size()) + "\r\n"
            "Connection: close\r\n\r\n";

    // mpv often aborts segment requests mid-flight, so bail the moment the socket is gone.
    if (sock.state() != QAbstractSocket::ConnectedState) return;
    sock.write(head);
    if (!headOnly) sock.write(body);
    while (sock.bytesToWrite() > 0 && sock.state() == QAbstractSocket::ConnectedState)
        if (!sock.waitForBytesWritten(10000)) break;
    if (sock.state() == QAbstractSocket::ConnectedState) {
        sock.disconnectFromHost();
        if (sock.state() != QAbstractSocket::UnconnectedState)
            sock.waitForDisconnected(3000);
    }
}

void HlsProxy::incomingConnection(qintptr handle) {
    const quint16 port = m_port.load();
    const QByteArray secret = m_secret;   // by value: the job must never touch `this`
    m_pool.start([handle, port, secret]() {
        QTcpSocket sock;
        if (!sock.setSocketDescriptor(handle)) {
            ::closesocket(SOCKET(handle));
            return;
        }

        QByteArray req;
        QElapsedTimer age;
        age.start();
        while (!req.contains("\r\n\r\n")) {
            if (req.size() > 8192 || age.elapsed() > 5000) return;   // junk, or a stalled client
            if (!sock.waitForReadyRead(1000)) {
                if (sock.state() != QAbstractSocket::ConnectedState) return;
                continue;
            }
            req += sock.readAll();
        }

        const QList<QByteArray> parts = req.left(req.indexOf("\r\n")).split(' ');
        if (parts.size() < 2) { reply(sock, 400, {}, {}); return; }
        const QByteArray method = parts[0];
        const QUrl target(QString::fromUtf8(parts[1]));
        const QUrlQuery q(target.query());
        const QString url = q.queryItemValue("u", QUrl::FullyDecoded);
        const QString ref = q.queryItemValue("r", QUrl::FullyDecoded);
        const bool isPlaylist = target.path().endsWith(".m3u8");
        const QByteArray ctype = isPlaylist ? "application/vnd.apple.mpegurl" : "video/mp2t";

        if (method != "GET" && method != "HEAD") { reply(sock, 405, {}, {}); return; }
        if (sign(secret, url, ref) != q.queryItemValue("s").toLatin1()) { reply(sock, 403, {}, {}); return; }
        const QString scheme = QUrl(url).scheme();
        if (scheme != "http" && scheme != "https") { reply(sock, 400, {}, {}); return; }
        // HEAD is only ever ServerSelector's reachability probe - answer it without pulling a body.
        if (method == "HEAD") { reply(sock, 200, ctype, {}, true); return; }

        // Playlists are rewritten whole so a Range on one is ignored; segments forward it.
        QByteArray range;
        for (const QByteArray &line : req.split('\n'))
            if (line.left(6).toLower() == "range:") range = line.mid(6).trimmed();

        const Fetched f = isPlaylist ? fetchPlaylist(url, ref) : fetch(url, ref, range);
        if (f.code < 200 || f.code >= 400) { reply(sock, f.code > 0 ? f.code : 502, {}, {}); return; }

        QByteArray body;
        if (isPlaylist)
            body = rewritePlaylist(QString::fromUtf8(f.data), url, ref, port, secret).toUtf8();
        else if (target.path().endsWith(".bin"))
            body = f.data;
        else
            body = f.data.isEmpty() ? f.data : f.data.mid(tsOffset(f.data));

        const bool partial = !isPlaylist && f.code == 206;
        reply(sock, partial ? 206 : 200, ctype, body, false, partial ? f.contentRange : QByteArray());
    });
}
