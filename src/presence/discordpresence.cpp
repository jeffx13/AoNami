#include "discordpresence.h"
#include "player/playlistitem.h"
#include "app/settings.h"
#include "app/logger.h"
#include <QCoreApplication>
#include <QDataStream>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <cmath>

DiscordPresence::DiscordPresence(QObject *parent) : QObject(parent) {
    m_clientId = Settings::instance().get(Config::DiscordClientId);

    connect(&m_socket, &QLocalSocket::connected,     this, &DiscordPresence::onConnected);
    connect(&m_socket, &QLocalSocket::readyRead,     this, &DiscordPresence::onReadyRead);
    connect(&m_socket, &QLocalSocket::disconnected,  this, &DiscordPresence::onDisconnected);
    connect(&m_socket, &QLocalSocket::errorOccurred, this, &DiscordPresence::onSocketError);

    m_reconnectTimer.setInterval(15000);
    connect(&m_reconnectTimer, &QTimer::timeout, this, &DiscordPresence::tryConnect);

    auto &settings = Settings::instance();
    connect(&settings, &Settings::discordEnabledChanged, this, [this]() {
        if (presenceEnabled()) tryConnect();
        else                   closeConnection();
    });
    // Pick up a client-id override edited directly in settings.ini.
    connect(&settings, &Settings::settingsChanged, this, [this]() {
        const QString id = Settings::instance().get(Config::DiscordClientId);
        if (id == m_clientId) return;
        m_clientId = id;
        closeConnection();
        if (presenceEnabled()) tryConnect();
    });

    if (presenceEnabled()) tryConnect();
}

DiscordPresence::~DiscordPresence() {
    // ~QLocalSocket emits disconnected(); stop it reaching our slots mid-teardown.
    disconnect(&m_socket, nullptr, this, nullptr);
}

bool DiscordPresence::presenceEnabled() const {
    return Settings::instance().get(Config::DiscordEnabled) && !m_clientId.isEmpty();
}

void DiscordPresence::tryConnect() {
    if (!presenceEnabled()) return;
    if (m_socket.state() != QLocalSocket::UnconnectedState) return;
    m_reconnectTimer.stop();
    m_pipeIndex = 0;
    m_socket.connectToServer(QStringLiteral("discord-ipc-0"));
}

void DiscordPresence::closeConnection() {
    m_reconnectTimer.stop();
    m_ready = false;
    m_buffer.clear();
    m_pipeIndex = 0;
    if (m_socket.state() != QLocalSocket::UnconnectedState)
        m_socket.abort();
}

void DiscordPresence::scheduleReconnect() {
    m_ready = false;
    m_buffer.clear();
    if (presenceEnabled() && !m_reconnectTimer.isActive())
        m_reconnectTimer.start();
}

void DiscordPresence::onConnected() {
    m_reconnectTimer.stop();
    m_ready = false;
    m_buffer.clear();
    sendFrame(0, QJsonObject{{"v", 1}, {"client_id", m_clientId}});   // op 0 = HANDSHAKE
}

void DiscordPresence::onReadyRead() {
    m_buffer.append(m_socket.readAll());
    while (m_buffer.size() >= 8) {
        qint32 op = 0, len = 0;
        { QDataStream hs(m_buffer.left(8)); hs.setByteOrder(QDataStream::LittleEndian); hs >> op >> len; }
        if (len < 0 || m_buffer.size() < 8 + len) break;
        const QByteArray payload = m_buffer.mid(8, len);
        m_buffer.remove(0, 8 + len);

        if (op == 2) {   // CLOSE - Discord rejected us (e.g. invalid client_id)
            oLog() << "Discord" << "presence rejected:"
                   << QJsonDocument::fromJson(payload).object().value("message").toString();
            closeConnection();
            return;
        }
        if (!m_ready) {   // first frame after the handshake is the READY dispatch
            m_ready = true;
            updateActivity();
        }
    }
}

void DiscordPresence::onDisconnected() {
    scheduleReconnect();
}

void DiscordPresence::onSocketError(QLocalSocket::LocalSocketError) {
    // Connect attempt failed - walk discord-ipc-0..9 looking for a live Discord.
    if (!m_ready && m_pipeIndex < 9) {
        m_socket.connectToServer(QStringLiteral("discord-ipc-") + QString::number(++m_pipeIndex));
        return;
    }
    scheduleReconnect();
}

void DiscordPresence::sendFrame(qint32 op, const QJsonObject &payload) {
    if (m_socket.state() != QLocalSocket::ConnectedState) return;
    const QByteArray data = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    QByteArray header;
    { QDataStream hs(&header, QIODevice::WriteOnly); hs.setByteOrder(QDataStream::LittleEndian);
      hs << op << static_cast<qint32>(data.size()); }
    m_socket.write(header);
    m_socket.write(data);
    m_socket.flush();
}

void DiscordPresence::onCurrentItemChanged(PlaylistItem *item) {
    if (!item || item->isList()) {
        m_details.clear();
        m_state.clear();
        m_startEpoch = 0;
    } else {
        auto parent = item->parent();
        m_details = parent && !parent->name.isEmpty() ? parent->name : item->displayName.simplified();
        if (item->number > 0) {
            const float n = item->number;
            m_state = QStringLiteral("Episode ") +
                      (std::floor(n) == n ? QString::number(static_cast<int>(n)) : QString::number(n, 'f', 1));
            if (item->season > 0)
                m_state = QStringLiteral("Season %1 ").arg(item->season) + m_state;
            const QString episodeName = item->name.simplified();
            if (!episodeName.isEmpty())
                m_state += QStringLiteral(" - ") + episodeName;
        } else {
            m_state = item->displayName.simplified();
        }
        m_startEpoch = QDateTime::currentSecsSinceEpoch();
    }

    if (m_ready) updateActivity();
    if (m_socket.state() == QLocalSocket::UnconnectedState) tryConnect();
}

void DiscordPresence::updateActivity() {
    if (!m_ready) return;

    QJsonObject activity;
    if (!m_details.isEmpty()) {
        activity["details"] = m_details;
        if (!m_state.isEmpty()) activity["state"] = m_state;
        activity["assets"] = QJsonObject{{"large_image", "logo"}, {"large_text", "AoNami"}};
        if (m_startEpoch > 0)
            activity["timestamps"] = QJsonObject{{"start", m_startEpoch}};
    }

    QJsonObject args{{"pid", static_cast<int>(QCoreApplication::applicationPid())}, {"activity", activity}};
    QJsonObject frame{
        {"cmd", "SET_ACTIVITY"},
        {"args", args},
        {"nonce", QString::number(QDateTime::currentMSecsSinceEpoch())}
    };
    sendFrame(1, frame);   // op 1 = FRAME
}
