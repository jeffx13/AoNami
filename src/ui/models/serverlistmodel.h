#pragma once


#include <QAbstractListModel>
#include <QFutureWatcher>
#include <QPair>
#include "base/network/network.h"
#include "providers/showprovider.h"

class ServerListModel : public QAbstractListModel {

    Q_OBJECT
    Q_PROPERTY(int currentIndex READ getCurrentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(int count        READ count                                 NOTIFY countChanged)
public:
    ServerListModel() {}
    ~ServerListModel() = default;

    void setServers(const QList<VideoServer> &servers, ShowProvider *provider);
    void setCurrentIndex(int index);
    void clear();
    VideoServer& at(int index);
    void setPreferredServer(int index);
    int count() const {
        return m_servers.count();
    }
    int getCurrentIndex() const { return m_currentIndex; }
    bool isValidIndex(int index) const;

    static bool checkVideo(Client *client, PlayInfo &playItem);
    static QPair<int, PlayInfo> findWorkingServer(Client* client, ShowProvider *provider, QList<VideoServer> &servers);
    PlayInfo loadServer(Client* client, int index);
signals:
    void currentIndexChanged();
    void countChanged();
private:
    int m_currentIndex = -1;
    QList<VideoServer> m_servers;
    ShowProvider *m_provider = nullptr;

    enum {
        NameRole = Qt::UserRole,
        LinkRole
    };
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
};



