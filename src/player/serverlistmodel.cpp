#include "serverlistmodel.h"
#include "providers/showprovider.h"
#include "utils/logger.h"
#include <QtConcurrent>


void ServerListModel::setServers(const QList<VideoServer> &servers, ShowProvider *provider) {
    m_servers = servers;
    m_provider = provider;
    emit layoutChanged();
}

void ServerListModel::setCurrentIndex(int index) {
    if (index == m_currentIndex || !isValidIndex(index)) return;
    m_currentIndex = index;
    emit currentIndexChanged();
}

bool ServerListModel::checkVideo(Client *client, PlayItem &playItem) {
    if (playItem.videos.isEmpty()) return false;
    return client->partialGet(playItem.videos.first().url.toString(), playItem.headers);
}

QPair<int, PlayItem> ServerListModel::findWorkingServer(Client *client, ShowProvider *provider, QList<VideoServer> &servers) {
    // preferredServerName = preferredServerName.isEmpty() ? provider->getPreferredServer() : preferredServerName;
    QString preferredServerName = provider->getPreferredServer();
    // Preferred server is the server that was used last time by the provider

    int index = -1;
    PlayItem playItem;
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
        QList<QFuture<void>> jobs;
        for (int i = 0; i < servers.size(); i++) {
            if (client->isCancelled()) break;
            jobs.push_back(QtConcurrent::run([i, &servers, client, provider, &index, &playItem]() {
                // new thread create new client
                Client subClient = *client;
                auto &server = servers[i];
                if (subClient.isCancelled() || index != -1) return;

                try {
                    auto serverPlayItem = provider->extractSource(&subClient, server);
                    if (subClient.isCancelled() || index != -1) return;

                    if (checkVideo(&subClient, serverPlayItem)) {
                        if (subClient.isCancelled() || index != -1) return;
                        gLog() << "Server" << "Using" << server.name;
                        index = i;
                        playItem = serverPlayItem;
                    } else {
                        oLog() << "Server" << QString("Server (%1)").arg(server.name) << "is broken";
                    }
                } catch (MyException &e) {
                    e.print();
                }
            }));
        }

        for (auto &job: jobs) {
            job.waitForFinished();
        }

    }

    return QPair<int, PlayItem>(index, playItem);
}


PlayItem ServerListModel::loadServer(Client *client, int index)
{
    if (!isValidIndex(index)|| !m_provider) return PlayItem();

    auto &server = m_servers[index];
    PlayItem playItem = m_provider->extractSource(client, server);

    if (checkVideo(client, playItem)) {
        cLog() << "Server" << "Using server" << server.name;
    } else {
        oLog() << "Server" << server.name << "is broken";
        playItem.clear();
        beginRemoveRows(QModelIndex(), index, index);
        m_servers.removeAt(index);
        endRemoveRows();
        if (m_currentIndex > index) {
            setCurrentIndex(m_currentIndex - 1);
        }

    }

    return playItem;
}



void ServerListModel::removeServerAt(int index) {
    if (!isValidIndex(index)) return;
    beginRemoveRows(QModelIndex(), index, index);
    m_servers.removeAt(index);
    endRemoveRows();
}



bool ServerListModel::isValidIndex(int index) const {
    return index >= 0 && index < m_servers.size();
}

int ServerListModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent);
    return m_servers.size();
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

// static QFutureWatcher<void> watcher;
// if (watcher.isRunning()) {
//     m_isCancelled = true;
//     watcher.waitForFinished();
// }
// m_isCancelled = false;
// auto future = QtConcurrent::run([provider, this](){
//     auto client = Client(&m_isCancelled);
//     QMutableListIterator<VideoServer> serverIterator(m_servers);
//     int index = -1;
//     while (serverIterator.hasNext()) {
//         if (m_isCancelled) return;
//         auto &server = serverIterator.next();
//         index++;
//         if (m_isCancelled) return;
//         auto playInfo = provider->extractSource(&client, server);
//         if (m_isCancelled) return;
//         checkSources(&client, playInfo.sources);
//         if (m_isCancelled) return;
//         if (playInfo.sources.isEmpty()){
//             oLog() << "Server" << server.name << "is broken";
//             beginRemoveRows(QModelIndex(), index, index);
//             serverIterator.remove();
//             endRemoveRows();
//             if (m_currentIndex > index) {
//                 setCurrentIndex(m_currentIndex - 1);
//             }
//         }
//     }
// });
// watcher.setFuture(future);
