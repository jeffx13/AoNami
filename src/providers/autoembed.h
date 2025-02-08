#pragma once
#include <QDebug>
#include "showprovider.h"
#include "network/csoup.h"
#include <QDateTime>
#include <QProcess>

class Autoembed : public ShowProvider
{
public:
    explicit Autoembed(QObject *parent = nullptr) : ShowProvider{parent} {}
    QString            name() const override { return "Autoembed"; }
    QString            hostUrl ()const override { return "https://watch.autoembed.cc"; }
    QList<int>         getAvailableTypes()const override { return {ShowData::TVSERIES}; }
    QList<ShowData>    search           (Client *client, const QString &query, int page, int type) override;
    QList<ShowData>    popular          (Client *client, int page, int type) override;
    QList<ShowData>    latest           (Client *client, int page, int type) override;
    int                loadDetails      (Client *client, ShowData &show, bool loadInfo, bool getPlaylist, bool getEpisodeCount) const override;
    QList<VideoServer> loadServers      (Client *client, const PlaylistItem* episode) const override;
    PlayInfo           extractSource    (Client *client, VideoServer& server) override;
};



