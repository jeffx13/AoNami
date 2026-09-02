#pragma once
#include <QTcpServer>
#include <QThreadPool>
#include <QByteArray>
#include <QString>
#include <atomic>

// Some CDNs prepend a fake PNG header to each .ts; this strips it before the first sync byte.
class HlsProxy : public QTcpServer {
    Q_OBJECT
public:
    explicit HlsProxy(QObject *parent = nullptr);
    ~HlsProxy() override;
    static HlsProxy *instance() { return s_instance; }

    // Entry url for a real m3u8 - the upstream Referer rides along in the query.
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
