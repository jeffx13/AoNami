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

    void setServers(const QList<VideoServer> &servers, ShowProvider *provider);

    void setCurrentIndex(int index);

    static void checkSources(Client *client, QList<Video> &sources);

    static PlayInfo autoSelectServer(Client* client, QList<VideoServer> &servers, ShowProvider *provider,const QString &preferredServerName = "");

    VideoServer &getServerAt(int index);

    void removeServerAt(int index);

    int getCurrentIndex() const { return m_currentIndex; }

    PlayInfo extract(Client* client, int index);
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



