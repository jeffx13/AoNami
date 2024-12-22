#pragma once

#include "providers/showprovider.h"

class Haitu : public ShowProvider
{
public:
    explicit Haitu(QObject *parent = nullptr) : ShowProvider(parent) {};
    QString name() const override { return "海兔影院"; }
    QString hostUrl() const override { return "https://www.haituu.tv/"; }
    QList<int> getAvailableTypes() const override {
        return {ShowData::ANIME, ShowData::MOVIE, ShowData::TVSERIES, ShowData::VARIETY};
    };

    QList<ShowData>    search       (Client *client, const QString &query, int page, int type) override;
    QList<ShowData>    popular      (Client *client, int page, int type) override;
    QList<ShowData>    latest       (Client *client, int page, int type) override;
    int                loadDetails  (Client *client, ShowData &show, bool loadInfo, bool getPlaylist, bool getEpisodeCount) const override;
    QList<VideoServer> loadServers  (Client *client, const PlaylistItem* episode) const override;
    PlayInfo           extractSource(Client *client, VideoServer &server) const override;
private:
    QList<ShowData>    filterSearch (Client *client, const QString &query, const QString &sortBy, int page);
    QMap<int, int> typesMap = {
        {ShowData::ANIME, 4},
        {ShowData::MOVIE, 1},
        {ShowData::TVSERIES, 2},
        {ShowData::VARIETY, 3}
    };
};
