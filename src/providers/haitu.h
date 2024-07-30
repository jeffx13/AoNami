#pragma once

#include "providers/showprovider.h"

class Haitu : public ShowProvider
{
    QRegularExpression player_aaaa_regex{R"(player_aaaa=(\{.*?\})</script>)"};
    QMap<int, int> typesMap = {
        {ShowData::ANIME, 4},
        {ShowData::MOVIE, 1},
        {ShowData::TVSERIES, 2},
        {ShowData::VARIETY, 3}
    };
public:
    explicit Haitu(QObject *parent = nullptr) : ShowProvider(parent) {};
public:
    QString name() const override { return "海兔影院"; }
    QString baseUrl = "https://www.haituu.tv/";
    inline QList<int> getAvailableTypes() const override {
        return {ShowData::ANIME, ShowData::MOVIE, ShowData::TVSERIES, ShowData::VARIETY};
    };

    QList<ShowData> search(const QString &query, int page, int type) override;
    QList<ShowData> popular(int page, int type) override;
    QList<ShowData> latest(int page, int type) override;
    QList<ShowData> filterSearch(const QString &query, const QString &sortBy, int page);

    bool loadDetails(ShowData &show, bool getPlaylist = true) const override;
    int getTotalEpisodes(const QString& link) const override
    {
        return 0;
    }
    QList<VideoServer> loadServers(const PlaylistItem* episode) const override;
    PlayInfo extractSource(const VideoServer &server) const override;
};
