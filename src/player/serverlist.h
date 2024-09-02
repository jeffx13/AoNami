#pragma once
#include "providers/showprovider.h"
class ServerList
{
    QList<VideoServer> m_servers;
    int m_currentIndex = -1;
    ShowProvider* m_provider = nullptr;

public:
    ServerList();

    void setServers(const QList<VideoServer> &servers, ShowProvider *provider);

    void removeAt(int index) {
        if (index > m_servers.size() || index < 0)
            return;
        m_servers.removeAt(index);
    };

    static PlayInfo autoSelectServer(Client* client, QList<VideoServer> &servers, ShowProvider *provider, const QString &preferredServerName = "");

    PlayInfo extract(Client* client, int index) {
        PlayInfo playInfo;
        QString serverName;
        if (isValidIndex(index)) {
            serverName = m_servers.at(index).name;
            qInfo() << "Log (Servers)    : Attempting to extract source from server" << serverName;
            playInfo = m_provider->extractSource(client, m_servers.at(index));
            playInfo.serverIndex = index;
        } else {
            // Auto select server
            playInfo = autoSelectServer(client, m_servers, m_provider);
            if (!playInfo.sources.isEmpty()) {
                serverName = m_servers.at(playInfo.serverIndex).name;
            } else {
                qInfo() << "Log (Servers)    : Failed to find a working server";
            }
        }

        playInfo.sources = checkSources(client, playInfo.sources);
        return playInfo;
    };

    static QList<Video> checkSources(Client* client, const QList<Video> &sources) {
        QList<Video> workingSources;
        for (auto& source : sources) {
            auto isWorking = client->isOk(source.videoUrl.toString());
            if (isWorking)
                workingSources.append(source);
        }

        if (workingSources.isEmpty())
            qWarning() << "Log (Servers)    : No working sources found";
        return workingSources;
    }

    bool setCurrentIndex(int index);

    int getCurrentIndex() const { return m_currentIndex; }

    QList<VideoServer> servers() const { return m_servers; }
    bool isEmpty() const { return m_servers.isEmpty(); }
    int size() const { return m_servers.size(); }

    VideoServer at(int index) const {
        if (!isValidIndex(index))
            throw std::out_of_range("Index out of range");
        return m_servers[index];
    }
    bool isValidIndex(int index) const {
        return index >= 0 && index < m_servers.size();
    }
    VideoServer operator[](int index) const { return m_servers[index]; }

};


