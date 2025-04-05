#pragma once

#include "providers/showprovider.h"

class Haitu : public ShowProvider
{
public:
    explicit Haitu(QObject *parent = nullptr) : ShowProvider(parent) {};
    QString name() const override { return "海兔影院"; }
    QString hostUrl() const override { return "https://www.haitu.xyz/"; }
    QList<QString> getAvailableTypes() const override {
        return {"动漫", "电影", "电视剧", "综艺"};
    };

    QList<ShowData>    search       (Client *client, const QString &query, int page, int type) override;
    QList<ShowData>    popular      (Client *client, int page, int type) override;
    QList<ShowData>    latest       (Client *client, int page, int type) override;
    int                loadDetails  (Client *client, ShowData &show, bool getEpisodeCountOnly, bool fetchPlaylist) const override;
    QList<VideoServer> loadServers  (Client *client, const PlaylistItem* episode) const override;
    PlayItem               extractSource(Client *client, VideoServer &server) override;
private:
    QList<ShowData>    filterSearch (Client *client, const QString &query, const QString &sortBy, int page);

    QList<int> types = {
        4, // 国创
        1, // 电影
        2, // 电视剧
        3, // 综艺
    };
};
