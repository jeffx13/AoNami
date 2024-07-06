#include "serverlistmodel.h"
#include "providers/showprovider.h"


void ServerListModel::setServers(const QList<VideoServer> &servers, ShowProvider *provider, int index) {
    if (!provider || servers.isEmpty()) return;
    m_provider = provider;
    m_servers = servers;
    m_currentIndex = index;
    m_subtitleListModel.clear();
    emit layoutChanged();
}

void ServerListModel::invalidateServer(int index) {
    // server is not working
    if (index > m_servers.size() || index < 0)
        return;
    m_servers[index].working = false;
}

PlayInfo ServerListModel::autoSelectServer(const QList<VideoServer> &servers, ShowProvider *provider,const QString &preferredServerName, bool selectServer) {
    QString preferredServer = preferredServerName.isEmpty() ? provider->getPreferredServer() : preferredServerName;
    PlayInfo playInfo;

    if (!preferredServer.isEmpty()) {
        for (int i = 0; i < servers.size(); i++) {
            if (servers[i].name == preferredServer) {
                auto& server = servers[i];
                qDebug() << "Log (Servers)    : Using preferred server" << server.name;
                playInfo = provider->extractSource(server);
                if (!playInfo.sources.isEmpty()) {
                    playInfo.serverIndex = i;
                    break;
                }
            }
        }
        if (playInfo.serverIndex == -1)
            qDebug() << "Log (Servers)    : Preferred server" << preferredServer << "not available for this episode";
    }

    if (playInfo.serverIndex == -1) {
        for (int i = 0; i < servers.size(); i++) {
            auto& server = servers.at (i);
            playInfo = provider->extractSource(server);
            if (!playInfo.sources.isEmpty()){
                provider->setPreferredServer(server.name);
                qDebug() << "Log (Servers)    : Using server" << server.name;
                playInfo.serverIndex = i;
                break;
            }
        }
    }

    if (selectServer && !playInfo.sources.isEmpty()) {
        setServers(servers, provider, playInfo.serverIndex);
        if (!playInfo.subtitles.isEmpty())
            m_subtitleListModel.setList(playInfo.subtitles);
    }
    return playInfo;
}

void ServerListModel::setCurrentIndex(int index) {
    if (index == m_currentIndex) return;
    int previousIndex = m_currentIndex;

    PlayInfo playInfo;
    QString serverName;

    if (index == -1) {
        playInfo = autoSelectServer(m_servers, m_provider);
        if (!playInfo.sources.isEmpty()) {
            m_currentIndex = playInfo.serverIndex;
            emit currentIndexChanged();
            serverName = m_servers.at(playInfo.serverIndex).name;
        } else {
            qInfo() << "Log (Servers)    : Failed to find a working server";
        }
    } else if (index >= 0 && index < m_servers.size()) {
        serverName = m_servers.at(index).name;
        qInfo() << "Log (Servers)    : Attempting to extract source from server" << serverName;
        playInfo = m_provider->extractSource(m_servers.at(index));
    };

    // auto playInfo = load(index);
    if (!playInfo.sources.isEmpty()) {
        m_currentIndex = index;
        emit currentIndexChanged();

        m_provider->setPreferredServer (serverName);
        qInfo() << "Log (Server): Fetched source" << playInfo.sources.first().videoUrl;
        MpvObject::instance()->open (playInfo.sources.first(), MpvObject::instance()->time());
        if (!playInfo.subtitles.isEmpty())
            m_subtitleListModel.setList(playInfo.subtitles);

    } else {
        m_currentIndex = previousIndex;
        emit currentIndexChanged();
        qWarning() << "Log (Servers)    : Failed to extract source from" << serverName;
    }
}


