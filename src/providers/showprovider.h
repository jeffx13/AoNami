#pragma once
#include "network/myexception.h"
#include "network/network.h"
#include "utils/functions.h"

#include "core/showdata.h"
#include "player/playlistitem.h"
#include "player/playinfo.h"

#include <QString>


class ShowProvider : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT);
protected:

    QString m_preferredServer;
    int resolveTitleNumber(QString &title) const {
        static auto replaceRegex = QRegularExpression("[第集话完结期]");
        float number = -1;
        bool ok;
        QString tempTitle = title;
        tempTitle.replace(replaceRegex, "");
        float floatTitle = tempTitle.toFloat(&ok);
        if (ok){
            number = floatTitle;
            title = "";
        }
        return number;
    }
public:
    ShowProvider(QObject *parent = nullptr) : QObject(parent){};
    virtual QString name() const = 0;
    QString baseUrl = "";
    virtual QList<int> getAvailableTypes() const = 0;

    virtual QList<ShowData>    search       (Client *client, const QString &query, int page, int type) = 0;
    virtual QList<ShowData>    popular      (Client *client, int page, int type) = 0;
    virtual QList<ShowData>    latest       (Client *client, int page, int type) = 0;
    virtual int                loadDetails  (Client *client, ShowData &show, bool loadInfo, bool getPlaylist, bool getEpisodeCount) const = 0;
    virtual QList<VideoServer> loadServers  (Client *client, const PlaylistItem *episode) const = 0;
    virtual PlayInfo           extractSource(Client *client, const VideoServer &server) const = 0;
    // virtual int getTotalEpisodes(const QString &link) const = 0;

    inline void setPreferredServer(const QString &serverName) {
        m_preferredServer = serverName;
    }

    [[nodiscard]] QString getPreferredServer() const {
        return m_preferredServer;
    }

};


