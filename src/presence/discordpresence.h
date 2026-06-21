#pragma once
#include <QObject>
#include <QLocalSocket>
#include <QTimer>
#include <QByteArray>

class PlaylistItem;
class QJsonObject;

// Shows the current episode as Discord activity over its local IPC pipe (no external lib).
class DiscordPresence : public QObject {
    Q_OBJECT
public:
    explicit DiscordPresence(QObject *parent = nullptr);

    void onCurrentItemChanged(PlaylistItem *item);   // from PlaylistManager::currentItemChanged

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

    QString  m_details;     // show title
    QString  m_state;       // "Episode N"
    qint64   m_startEpoch = 0;
};
