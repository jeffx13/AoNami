#include "serverlistmodel.h"
#include "providers/showprovider.h"
#include "app/logger.h"
#include <QtConcurrent/QtConcurrentRun>

void ServerListModel::setServers(const QList<VideoServer> &servers, ShowProvider *provider) {
    m_servers = servers;
    m_provider = provider;
    emit countChanged();
    emit layoutChanged();
}

void ServerListModel::setCurrentIndex(int index) {
    if (index == m_currentIndex || !isValidIndex(index)) return;
    m_currentIndex = index;
    emit currentIndexChanged();
}

void ServerListModel::setPreferredServer(int index) {
    if (!m_provider|| !isValidIndex(index)) return;
    m_provider->setPreferredServer(m_servers[index].name);
}

VideoServer &ServerListModel::at(int index) {
    return m_servers[index];
}

void ServerListModel::clear() {
    m_servers.clear();
    m_currentIndex = -1;
    m_provider = nullptr;
    emit countChanged();
    emit layoutChanged();
}

bool ServerListModel::checkVideo(Client *client, PlayInfo &playItem) {
    if (playItem.videos.isEmpty()) return false;
    return client->partialGet(playItem.videos.first().url.toString(), playItem.headers);
}

QPair<int, PlayInfo> ServerListModel::findWorkingServer(Client *client, ShowProvider *provider, QList<VideoServer> &servers) {
    // preferredServerName = preferredServerName.isEmpty() ? provider->getPreferredServer() : preferredServerName;
    QString preferredServerName = provider->getPreferredServer();
    // Preferred server is the server that was used last time by the provider

    int index = -1;
    PlayInfo playItem;
    if (!preferredServerName.isEmpty()) {
        // Find the index of preferred server
        auto preferredServer = std::find_if(servers.begin(), servers.end(), [&preferredServerName](const VideoServer &server) {
            return server.name == preferredServerName;
        });

        if (preferredServer != servers.end()) {
            index = std::distance(servers.begin(), preferredServer);
            auto &server = *preferredServer;
            playItem = provider->extractSource(client, server);
            if (checkVideo(client, playItem)) {
                gLog() << "Server" << "Using preferred server" << server.name;
            } else {
                oLog() << QString("Preferred server (%1)").arg(server.name) << "is broken";
                servers.removeAt(index);
                playItem.clear();
                index = -1;
            }
        }
    }

    // if preferred server is not used, find a working server
    if (index == -1) {
        QList<QFuture<bool>> jobs;
        for (int i = 0; i < servers.size(); i++) {
            if (client->isCancelled()) break;
            jobs.push_back(QtConcurrent::run([i, &servers, client, provider, &index, &playItem]() {
                // new thread create new client
                Client subClient = *client;
                auto &server = servers[i];
                if (subClient.isCancelled() || index != -1) return true;

                try {
                    auto serverPlayItem = provider->extractSource(&subClient, server);
                    if (subClient.isCancelled() || index != -1) return true;

                    if (checkVideo(&subClient, serverPlayItem)) {
                        if (subClient.isCancelled() || index != -1) return true;
                        gLog() << "Server" << "Using" << server.name;
                        index = i;
                        playItem = serverPlayItem;
                    } else {
                        oLog() << "Server" << QString("Server (%1)").arg(server.name) << "is broken";
                        return false;
                    }
                } catch (MyException &e) {
                    e.print();
                }
                return true;
            }));
        }


        for (int i = jobs.size() - 1; i > -1; i--) {
            jobs[i].waitForFinished();
            bool isWorking = jobs[i].result();
            if (!isWorking) {
                servers.remove(i);
                if (index > i) {
                    index--;
                }
            }

        }

    }

    return QPair<int, PlayInfo>(index, playItem);
}


PlayInfo ServerListModel::loadServer(Client *client, int index)
{
    if (!isValidIndex(index)|| !m_provider) return PlayInfo();

    auto &server = m_servers[index];
    PlayInfo playItem = m_provider->extractSource(client, server);

    if (checkVideo(client, playItem)) {
        cLog() << "Server" << "Using server" << server.name;
    } else if (!client->isCancelled()){
        oLog() << "Server" << server.name << "is broken";
        playItem.clear();
        beginRemoveRows(QModelIndex(), index, index);
        m_servers.removeAt(index);
        endRemoveRows();
        emit countChanged();
        if (m_currentIndex > index) {
            setCurrentIndex(m_currentIndex - 1);
        }

    }

    return playItem;
}


bool ServerListModel::isValidIndex(int index) const {
    return index >= 0 && index < m_servers.size();
}

int ServerListModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent);
    return count();
}

QVariant ServerListModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid())
        return QVariant();

    auto server = m_servers.at(index.row());
    switch (role) {
    case NameRole:
    case Qt::DisplayRole:
        return server.name;
        break;
    case LinkRole:
        return server.link;
        break;
    default:
        break;
    }
    return QVariant();
}

QHash<int, QByteArray> ServerListModel::roleNames() const{
    QHash<int, QByteArray> names;
    names[NameRole] = "name";
    names[LinkRole] = "link";
    names[Qt::DisplayRole] = "text";
    return names;
}
