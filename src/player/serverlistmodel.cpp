#include "serverlistmodel.h"


void ServerListModel::setServers(const QList<VideoServer> &servers, ShowProvider *provider, int index) {
    if (!provider || servers.isEmpty()) return;
    m_provider = provider;
    m_servers = servers;
    m_currentIndex = index;
    emit layoutChanged();
}

void ServerListModel::invalidateServer(int index) {
    // server is not working
    if (index > m_servers.size() || index < 0)
        return;
    m_servers[index].working = false;
}

QPair<QList<Video>, int> ServerListModel::autoSelectServer(const QList<VideoServer> &servers, ShowProvider *provider, const QString &preferredServerName) {
    QString preferredServer = preferredServerName.isEmpty() ? provider->getPreferredServer() : preferredServerName;
    QList<Video> videos;
    int serverIndex = -1;

    if (!preferredServer.isEmpty()) {
        for (int i = 0; i < servers.size(); i++) {
            if (servers[i].name == preferredServer) {
                auto& server = servers.at (i);
                qDebug() << "Log (Servers)    : Using preferred server" << server.name;
                videos = provider->extractSource(server);
                if (!videos.isEmpty()) {
                    serverIndex = i;
                    break;
                }
            }
        }
        if (serverIndex == -1)
            qDebug() << "Log (Servers)    : Preferred server" << preferredServer << "not available for this episode";
    }

    if (serverIndex == -1) {
        for (int i = 0; i < servers.size(); i++) {
            auto& server = servers.at (i);
            videos = provider->extractSource(server);
            if (!videos.isEmpty()){
                provider->setPreferredServer (server.name);
                qDebug() << "Log (Servers)    : Using server" << server.name;
                serverIndex = i;
                break;
            }
        }
    }

    return { videos, serverIndex};
}

QList<Video> ServerListModel::load(int index) {
    // auto select a working server
    QList<Video> videos;
    if (index == -1) {
        // videos = autoSelectServer();
        auto sourceAndIndex = autoSelectServer(m_servers, m_provider);
        videos = sourceAndIndex.first;
        if (!videos.isEmpty()) {
            m_currentIndex = sourceAndIndex.second;
            emit currentIndexChanged();
        } else {
            qInfo() << "Log (Servers)    : Failed to find a working server";
        }
        return videos;
    } else if (index < 0 || index >= m_servers.size()) return {};

    auto& server = m_servers.at (index);
    qInfo() << "Log (Servers)    : Attempting to extract source from server" << server.name;
    videos = m_provider->extractSource(server);
    m_currentIndex = index;
    emit currentIndexChanged();

    if (videos.isEmpty()) {
        qWarning() << "Log (Servers)    : Failed to extract source from" << server.name;
    } else {
        m_provider->setPreferredServer (server.name);
    }
    return videos;
}

