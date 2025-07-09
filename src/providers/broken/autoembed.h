// #pragma once
// #include <QDebug>
// #include "showprovider.h"
// #include "network/csoup.h"
// #include <QDateTime>
// #include <QProcess>

// class Autoembed : public ShowProvider
// {
// public:
//     explicit Autoembed(QObject *parent = nullptr) : ShowProvider{parent} {}
//     QString            name() const override { return "Autoembed"; }
//     QString            hostUrl ()const override { return "https://watch.autoembed.cc"; }
//     QList<QString>     getAvailableTypes()const override { return {"Movies", "Tv Series"}; }
//     QList<ShowData>    search           (Client *client, const QString &query, int page, int type) override;
//     QList<ShowData>    popular          (Client *client, int page, int typeIndex) override;
//     QList<ShowData>    latest           (Client *client, int page, int typeIndex) override;
//     int                loadDetails      (Client *client, ShowData &show, bool getEpisodeCountOnly, bool fetchPlaylist) const override;
//     QList<VideoServer> loadServers      (Client *client, const PlaylistItem* episode) const override;
//     PlayItem           extractSource    (Client *client, VideoServer& server) override;
// };



