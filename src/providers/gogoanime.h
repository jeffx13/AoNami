#pragma once
#include <QDebug>
#include "showprovider.h"
#include "network/csoup.h"


class Gogoanime : public ShowProvider
{
public:
    Gogoanime() = default;
    QString name() const override { return "Anitaku"; }
    QString baseUrl = "https://anitaku.to/";
    QList<int> getAvailableTypes() const override { return {ShowData::ANIME}; }

    QList<ShowData>    search       (Client *client, const QString &query, int page, int type) override;
    QList<ShowData>    popular      (Client *client, int page, int type) override;
    QList<ShowData>    latest       (Client *client, int page, int type) override;
    int                loadDetails  (Client *client, ShowData &show, bool loadInfo, bool loadPlaylist, bool getEpisodeCount) const override;
    QList<VideoServer> loadServers  (Client *client, const PlaylistItem* episode) const override;
    PlayInfo           extractSource(Client *client, const VideoServer& server) const override;
private:
    CSoup getInfoPage(const QString& link) const;
    QString getEpisodesLink(const CSoup &doc) const;
};



