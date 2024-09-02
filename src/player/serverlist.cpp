#include "serverlist.h"

#include <QtConcurrent>

ServerList::ServerList() {

}

void ServerList::setServers(const QList<VideoServer> &servers, ShowProvider *provider) {
    if (!provider || servers.isEmpty()) return;
    m_provider = provider;
    m_servers = servers;
}

PlayInfo ServerList::autoSelectServer(Client* client, QList<VideoServer> &servers, ShowProvider *provider,const QString &preferredServerName) {
    QString preferredServer = preferredServerName.isEmpty() ? provider->getPreferredServer() : preferredServerName;
    PlayInfo playInfo;
    QList<int> notWorking;
    if (!preferredServer.isEmpty()) {
        for (int i = 0; i < servers.size(); i++) {
            auto& server = servers[i];
            if (server.name == preferredServer) {
                // Extract the preferred server
                qDebug() << "Log (Servers)    : Extracting preferred server" << server.name;
                playInfo = provider->extractSource(client, server);
                playInfo.sources = checkSources(client, playInfo.sources);
                if (!playInfo.sources.isEmpty()){
                    playInfo.serverIndex = i;
                    servers[i].working = true;
                } else {
                    notWorking.append(i);
                    qDebug() << "Log (Servers)    : Preferred server" << preferredServer << "not available for this episode";
                }

            }
        }
    }
    if (playInfo.serverIndex == -1) {
        // Finding a working server
        for (int i = 0; i < servers.size(); i++) {
            playInfo = provider->extractSource(client, servers[i]);
            playInfo.sources = checkSources(client, playInfo.sources);
            if (!playInfo.sources.isEmpty()){
                provider->setPreferredServer(servers[i].name);
                playInfo.serverIndex = i;
                servers[i].working = true;
                qDebug() << "Log (Servers)    : Using server" << servers[i].name;
                break;
            }
        }
    }

    for (int i = notWorking.size() - 1; i >= 0; i--) {
        int index = notWorking[i];
        servers.removeAt(index);
        if (playInfo.serverIndex != -1 && index < playInfo.serverIndex)
            playInfo.serverIndex--;
    }

    return playInfo;
}

bool ServerList::setCurrentIndex(int index) {
    if (index == m_currentIndex) return true;
    if (!isValidIndex(index)) return false;
    m_currentIndex = index;
    return true;
}

