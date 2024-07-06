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
    QList<int> getAvailableTypes() const override {
        return {ShowData::ANIME};
    };

    QList<ShowData> search(const QString &query, int page, int type = 0) override;
    QList<ShowData> popular(int page, int type = 0) override;
    QList<ShowData> latest(int page, int type = 0) override;

    bool loadDetails(ShowData &show, bool getPlaylist = true) const override;
    int getTotalEpisodes(const QString& link) const override;
    QList<VideoServer> loadServers(const PlaylistItem* episode) const override;
    PlayInfo extractSource(const VideoServer& server) const override;
private:
    CSoup getInfoPage(const QString& link) const;
    QString getEpisodesLink(const CSoup &doc) const;
};



