#pragma once

#include "player/playlistitem.h"

#include "serverlist.h"

#include <QAbstractListModel>
#include <QFutureWatcher>
#include <QPair>
#include <QtConcurrent>

class ServerListModel : public QAbstractListModel {

    Q_OBJECT
    Q_PROPERTY(int currentIndex READ getCurrentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    std::atomic<bool> m_isCancelled = false;
public:
    ServerListModel() {}
    ~ServerListModel() = default;

    void setServers(QList<VideoServer> servers, ShowProvider *provider) {
        if (!provider || servers.isEmpty()) return;
        m_serverList.setServers(servers, provider);
        static QFutureWatcher<void> watcher;

        if (watcher.isRunning()) {
            m_isCancelled = true;
            watcher.waitForFinished();
        }
        m_isCancelled = false;

        auto future = QtConcurrent::run([provider, this](){
            QList<int> brokenServers;
            auto client = Client(&m_isCancelled);
            for (int i = m_serverList.size() - 1; i >= 0; i--) {
                if (m_serverList[i].working) continue;
                if (m_isCancelled) return;
                auto playInfo = provider->extractSource(&client, m_serverList[i]);
                if (m_isCancelled) return;
                playInfo.sources = m_serverList.checkSources(&client, playInfo.sources);
                if (m_isCancelled) return;
                if (playInfo.sources.isEmpty()) {
                    brokenServers.append(i);
                    qDebug() << "Log (Servers)    : Server" << m_serverList[i].name << "is not working";
                }
                m_serverList.removeAt(i);
                if (m_serverList.getCurrentIndex() > i)
                    m_serverList.setCurrentIndex(m_serverList.getCurrentIndex() - 1);
                emit layoutChanged();
            }
        });
        watcher.setFuture(future);
        emit layoutChanged();
    }

    void setCurrentIndex(int index) {
        m_serverList.setCurrentIndex(index);
        emit currentIndexChanged();
    }

    int getCurrentIndex() const { return m_serverList.getCurrentIndex(); }
    ServerList& getServerList() { return m_serverList; }

signals:
    void currentIndexChanged();
private:
    ServerList m_serverList;

    enum {
        NameRole = Qt::UserRole,
        LinkRole
    };
    int rowCount(const QModelIndex &parent = QModelIndex()) const override {
        return m_serverList.size();
    };

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        if (!index.isValid() || !m_serverList.isValidIndex(index.row()))
            return QVariant();

        const VideoServer &server = m_serverList[index.row()];
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



