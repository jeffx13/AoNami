#pragma once
#include <QDebug>
#include "showprovider.h"
#include "network/csoup.h"


class Wolong : public ShowProvider
{
public:
    Wolong() = default;
    QString name() const override { return "卧龙"; }
    QString baseUrl = "https://collect.wolongzy.cc/api.php/provide/vod/?";
    // https://collect.wolongzy.cc/api.php/provide/vod/?ac=list
    QList<int> getAvailableTypes() const override { return {ShowData::ANIME, ShowData::TVSERIES, ShowData::MOVIE}; }

    QList<ShowData>    search       (Client *client, const QString &query, int page, int type) override;
    QList<ShowData>    popular      (Client *client, int page, int type) override;
    QList<ShowData>    latest       (Client *client, int page, int type) override;
    int                loadDetails  (Client *client, ShowData &show, bool loadInfo, bool loadPlaylist, bool getEpisodeCount) const override;
    QList<VideoServer> loadServers  (Client *client, const PlaylistItem* episode) const override;
    PlayInfo           extractSource(Client *client, const VideoServer& server) const override;
private:
    QMap<int, int> typeMap {
                           {ShowData::ANIME, 25},
                           {ShowData::TVSERIES, 2},
                           {ShowData::MOVIE, 5},
                           };
};



