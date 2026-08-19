#pragma once
#include <QTcpServer>
#include <QThreadPool>
#include <QByteArray>
#include <QString>
#include <atomic>

// Some CDNs (e.g. Anikoto's megacloud embeds) prepend a fake PNG header to every HLS .ts segment
// so scrapers and players see "image/png" and fail. This loopback proxy serves the whole chain over
// http - so ffmpeg and mpv don't block cross-protocol segments - rewriting playlists and stripping
// the junk ahead of each segment's first MPEG-TS sync byte.
class HlsProxy : public QTcpServer {
    Q_OBJECT
public:
    explicit HlsProxy(QObject *parent = nullptr);
    ~HlsProxy() override;
    static HlsProxy *instance() { return s_instance; }

    // Entry url for a real m3u8, master or media - point the player at this. The upstream Referer
    // rides along in the query. Returns the original url if the proxy isn't listening.
    QString playlistUrl(const QString &m3u8Url, const QString &referer);

protected:
    void incomingConnection(qintptr handle) override;

private:
    void ensureListening();
    std::atomic<quint16> m_port{0};
    QByteArray  m_secret;   // set once in the ctor, only read afterwards
    QThreadPool m_pool;     // segment fetches block for seconds - keep them off the global pool
    static HlsProxy *s_instance;
};
