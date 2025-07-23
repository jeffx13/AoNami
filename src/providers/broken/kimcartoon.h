// #pragma once
// #include "Providers/showprovider.h"
// #include "network/csoup.h"
// class Kimcartoon : public ShowProvider {
// public:
//     explicit Kimcartoon(QObject *parent = nullptr) : ShowProvider{parent} {};
//     QString hostUrl() const override { return "https://kimcartoon.si/"; }
//     QString              name() const override { return "KIMCartoon"; }
//     QList<int>           getAvailableTypes() const override { return {ShowData::ANIME}; }
//     QVector<ShowData>    search       (Client *client, const QString &query, int page, int type) override;
//     QVector<ShowData>    popular      (Client *client, int page, int typeIndex) override;
//     QVector<ShowData>    latest       (Client *client, int page, int typeIndex) override;
//     int                  loadDetails  (Client *client, ShowData &show, bool getEpisodeCountOnly, bool getPlaylist, bool getInfo = true) const override;
//     QVector<VideoServer> loadServers  (Client *client, const PlaylistItem *episode) const override;
//     PlayItem             extractSource(Client *client, VideoServer &server) override;
// private:
//     QVector<ShowData>    parseResults(const CSoup &doc);
// };


