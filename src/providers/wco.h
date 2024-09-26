#pragma once
#include <QDebug>
#include "showprovider.h"
#include "network/csoup.h"
#include <QDateTime>
#include <QProcess>

class WCOFun : public ShowProvider
{
public:
    explicit WCOFun(QObject *parent = nullptr) : ShowProvider{parent} {};
    QString baseUrl = "https://wcofun.net/";
    QString            name() const override { return "WCOFun"; }
    QList<int>         getAvailableTypes() const override { return {ShowData::ANIME}; }
    QList<ShowData>    search       (Client *client, const QString &query, int page, int type) override;
    QList<ShowData>    popular      (Client *client, int page, int type) override { return {}; }
    QList<ShowData>    latest       (Client *client, int page, int type) override;
    int                loadDetails  (Client *client, ShowData &show, bool loadInfo, bool getPlaylist, bool getEpisodeCount) const override;
    QList<VideoServer> loadServers  (Client *client, const PlaylistItem* episode) const override;
    PlayInfo           extractSource(Client *client, const VideoServer& server) const override;

};



