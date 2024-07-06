#pragma once

#include "data/playlistitem.h"
#include "player/mpvObject.h"
#include "subtitlelistmodel.h"
#include <QAbstractListModel>
#include <QPair>

class ServerListModel : public QAbstractListModel {

    Q_OBJECT
    Q_PROPERTY(int currentIndex READ getCurrentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
public:
    ServerListModel() = default;
    ~ServerListModel() = default;


    void setServers(const QList<VideoServer>& servers, ShowProvider* provider, int index);

    void invalidateServer(int index);

    PlayInfo autoSelectServer(const QList<VideoServer> &servers, ShowProvider *provider, const QString &preferredServerName = "", bool selectServer = true);

    void setCurrentIndex(int index);

    int getCurrentIndex() const { return m_currentIndex; }

    QList<VideoServer> servers() const { return m_servers; }
    bool isEmpty() const { return m_servers.isEmpty(); }
    int size() const { return m_servers.size(); }

    SubtitleListModel* getSubtitleList() {
        return &m_subtitleListModel;
    }

signals:
    void currentIndexChanged();
private:
    SubtitleListModel m_subtitleListModel;


    QList<VideoServer> m_servers;
    int m_currentIndex = -1;
    ShowProvider* m_provider = nullptr;

    enum {
        NameRole = Qt::UserRole,
        LinkRole
    };
    int rowCount(const QModelIndex &parent = QModelIndex()) const override {
        return m_servers.size();
    };
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        if (!index.isValid() || m_servers.isEmpty())
            return QVariant();

        const VideoServer &server = m_servers.at(index.row());
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



