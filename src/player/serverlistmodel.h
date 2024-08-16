#pragma once

#include "player/playlistitem.h"

#include "serverlist.h"

#include <QAbstractListModel>
#include <QPair>

class ServerListModel : public QAbstractListModel {

    Q_OBJECT
    Q_PROPERTY(int currentIndex READ getCurrentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
public:
    ServerListModel(std::atomic<bool> *shouldCancel) : m_serverList(shouldCancel) {

    }
    ~ServerListModel() = default;

    void setServers(QList<VideoServer> servers, ShowProvider *provider) {
        if (!provider || servers.isEmpty()) return;
        m_serverList.setServers(servers, provider);
        setCurrentIndex(0);
        emit layoutChanged();
    }

    void setCurrentIndex(int index) {
        m_serverList.setCurrentIndex(index);
        emit currentIndexChanged();
    }

    int getCurrentIndex() const { return m_serverList.getCurrentIndex(); }
    const ServerList& getServerList() { return m_serverList; }

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



