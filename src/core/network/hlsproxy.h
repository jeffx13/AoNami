#pragma once
#include <QTcpServer>
#include <QString>
#include <atomic>

// Some CDNs (e.g. Anikoto's megacloud embeds) prepend a fake PNG header to every HLS .ts segment so
// scrapers/players see "image/png" and fail. This loopback proxy serves the whole HLS chain over http
// (so ffmpeg/mpv don't block cross-protocol segments), rewriting playlists and stripping the junk
// before the first MPEG-TS sync byte on each segment.
class HlsProxy : public QTcpServer {
    Q_OBJECT
public:
    explicit HlsProxy(QObject *parent = nullptr);
    static HlsProxy *instance() { return s_instance; }

    // The proxy entry url for a real m3u8 (master or media). Point the player at this. The upstream
    // Referer rides along in the query. Returns the original url if the proxy isn't listening.
    QString playlistUrl(const QString &m3u8Url, const QString &referer);

protected:
    void incomingConnection(qintptr handle) override;

private:
    void ensureListening();
    std::atomic<quint16> m_port{0};
    static HlsProxy *s_instance;
};
