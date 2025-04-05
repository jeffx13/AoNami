#pragma once
#include "player/playlistitem.h"
#include "providers/showprovider.h"
#include <QAbstractListModel>
#include <QFutureWatcher>
#include <QPair>





class ServerListModel : public QAbstractListModel {

    Q_OBJECT
    Q_PROPERTY(int currentIndex READ getCurrentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(int count        READ rowCount                              NOTIFY layoutChanged)
    int m_currentIndex = -1;


    QList<VideoServer> m_servers;
    ShowProvider *m_provider = nullptr;
public:
    ServerListModel() {}
    ~ServerListModel() = default;

    void setServers(const QList<VideoServer> &servers, ShowProvider *provider);
    void setCurrentIndex(int index);
    void setPreferredServer(int index) {
        if (!m_provider|| !isValidIndex(index)) return;
        m_provider->setPreferredServer(m_servers[index].name);
    }
    VideoServer& getServerAt(int index) {
        return m_servers[index];
    }

    static bool checkVideo(Client *client, PlayItem &playItem);
    static QPair<int, PlayItem> findWorkingServer(Client* client, ShowProvider *provider, QList<VideoServer> &servers);
    PlayItem loadServer(Client* client, int index);
    void removeServerAt(int index);
    int getCurrentIndex() const { return m_currentIndex; }
    bool isValidIndex(int index) const;
signals:
    void currentIndexChanged();
private:
    enum {
        NameRole = Qt::UserRole,
        LinkRole
    };
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
};



