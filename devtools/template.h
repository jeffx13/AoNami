#pragma once
#include "providers/showprovider.h"

class NewProvider : public ShowProvider
{
public:
    NewProvider() = default;
    QString name() const override { return ""; }
    QString hostUrl = "https://";
    QList<int> getAvailableTypes() const override {
        return {ShowData::ANIME};
    };

    QList<ShowData> search(const QString &query, int page, int type = 0) override;
    QList<ShowData> popular(int page, int type = 0) override;
    QList<ShowData> latest(int page, int type = 0) override;

    bool loadDetails(ShowData& show, bool getPlaylist = true) const override;
    int getTotalEpisodes(const QString &link) const override;
    QList<VideoServer> loadServers(const PlaylistItem* episode) const override;
    QList<Video> extractSource(const VideoServer &server) const override;
};