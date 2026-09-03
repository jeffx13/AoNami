#pragma once
#include <QObject>
#include <QLocalSocket>
#include <QTimer>
#include <QByteArray>

class PlaylistItem;
class QJsonObject;

class DiscordPresence : public QObject {
    Q_OBJECT
public:
    explicit DiscordPresence(QObject *parent = nullptr);
    ~DiscordPresence();

    void onCurrentItemChanged(PlaylistItem *item);

private:
    bool presenceEnabled() const;
    void tryConnect();
    void closeConnection();
    void scheduleReconnect();
    void onConnected();
    void onReadyRead();
    void onDisconnected();
    void onSocketError(QLocalSocket::LocalSocketError);
    void sendFrame(qint32 op, const QJsonObject &payload);
    void updateActivity();

    QLocalSocket m_socket;
    QTimer       m_reconnectTimer;
    QByteArray   m_buffer;
    QString      m_clientId;
    int          m_pipeIndex = 0;   // Discord exposes discord-ipc-0..9
    bool         m_ready = false;

    QString  m_details;
    QString  m_state;
    qint64   m_startEpoch = 0;
};
