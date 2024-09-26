#pragma once

#include "player/playlistitem.h"

// #include "serverlist.h"
#include "providers/showprovider.h"
#include <QAbstractListModel>
#include <QFutureWatcher>
#include <QPair>
#include <QtConcurrent>

class ServerListModel : public QAbstractListModel {

    Q_OBJECT
    Q_PROPERTY(int currentIndex READ getCurrentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    std::atomic<bool> m_isCancelled = false;
    QList<VideoServer> m_servers;
    int m_currentIndex = -1;
    ShowProvider* m_provider = nullptr;
public:
    ServerListModel() {}
    ~ServerListModel() = default;

    void setServers(const QList<VideoServer> &servers, ShowProvider *provider) {
        if (!provider || servers.isEmpty()) return;
        m_provider = provider;
        m_servers = servers;
        static QFutureWatcher<void> watcher;

        if (watcher.isRunning()) {
            m_isCancelled = true;
            watcher.waitForFinished();
        }
        m_isCancelled = false;
        emit layoutChanged();
        return;
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
        //             qDebug() << "Log (Servers)    : Server" << server.name << "is broken";
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
    }

    void setCurrentIndex(int index) {
        if (index == m_currentIndex || !isValidIndex(index)) return;
        m_currentIndex = index;
        emit currentIndexChanged();
    }

    static void checkSources(Client *client, QList<Video> &sources) {
        QMutableListIterator<Video> sourceIterator(sources);
        while (sourceIterator.hasNext()) {
            auto video = sourceIterator.next();
            if (!client->isOk(video.videoUrl.toString(), video.getHeaders()))
                sourceIterator.remove();
        }
    }

    static PlayInfo autoSelectServer(Client* client, QList<VideoServer> &servers, ShowProvider *provider,const QString &preferredServerName = "") {
        QString preferredServer = preferredServerName.isEmpty() ? provider->getPreferredServer() : preferredServerName;
        PlayInfo playInfo;
        if (!preferredServer.isEmpty()) {
            QMutableListIterator<VideoServer> serverIterator(servers);
            int index = -1;
            while (serverIterator.hasNext()) {
                auto &server = serverIterator.next();
                index++;
                if (server.name != preferredServer) continue;
                playInfo = provider->extractSource(client, server);
                checkSources(client, playInfo.sources);
                if (!playInfo.sources.isEmpty()){
                    playInfo.serverIndex = index;
                    qDebug() << "Log (Servers)    : Using preferred server" << server.name;
                    return playInfo;
                } else {
                    qDebug() << "Log (Servers)    : Preferred server" << preferredServer << "is broken";
                    serverIterator.remove();
                }
            }
        }


        QMutableListIterator<VideoServer> serverIterator(servers);
        int index = -1;
        while (serverIterator.hasNext()) {
            auto &server = serverIterator.next();
            index++;
            playInfo = provider->extractSource(client, server);
            checkSources(client, playInfo.sources);
            if (!playInfo.sources.isEmpty()){
                provider->setPreferredServer(server.name);
                playInfo.serverIndex = index;
                qDebug() << "Log (Servers)    : Using server" << server.name;
                break;
            } else {
                qDebug() << "Log (Servers)    : Server" << server.name << "is broken";
                serverIterator.remove();
            }

        }

        return playInfo;
    }
    VideoServer &getServerAt(int index) {
        if (!isValidIndex(index))
            throw std::out_of_range("Index out of range");
        return m_servers[index];
    }

    void removeServerAt(int index) {
        if (!isValidIndex(index)) return;
        beginRemoveRows(QModelIndex(), index, index);
        m_servers.removeAt(index);
        endRemoveRows();
    }

    int getCurrentIndex() const { return m_currentIndex; }
    // ServerList& getServerList() { return m_serverList; }
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
            playInfo = autoSelectServer(client, m_servers, m_provider, "");
            if (!playInfo.sources.isEmpty()) {
                serverName = m_servers.at(playInfo.serverIndex).name;
            } else {
                qInfo() << "Log (Servers)    : Failed to find a working server";
            }
        }

        checkSources(client, playInfo.sources);
        return playInfo;
    };
    bool isValidIndex(int index) const {
        return index >= 0 && index < m_servers.size();
    }

signals:
    void currentIndexChanged();
private:
    // ServerList m_serverList;

    enum {
        NameRole = Qt::UserRole,
        LinkRole
    };
    int rowCount(const QModelIndex &parent = QModelIndex()) const override {
        return m_servers.size();
    };

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        if (!index.isValid())
            return QVariant();

        const VideoServer &server = m_servers[index.row()];
        switch (role) {
        case NameRole:
            return server.name;
            break;
        case LinkRole:
            return server.link;
            break;
        default:
            break;
        }
        return QVariant();
    };
    QHash<int, QByteArray> roleNames() const override{
        QHash<int, QByteArray> names;
        names[NameRole] = "name";
        names[LinkRole] = "link";
        return names;
    };
};



