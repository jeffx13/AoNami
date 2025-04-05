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

    QString hostUrl() const override { return "https://wcofun.net/"; }
    QString            name() const override { return "WCOFun"; }
    QList<QString>     getAvailableTypes() const override { return {"Cartoons"}; }
    QList<ShowData>    search       (Client *client, const QString &query, int page, int type) override;
    QList<ShowData>    popular      (Client *client, int page, int type) override { return {}; }
    QList<ShowData>    latest       (Client *client, int page, int type) override;
    int                loadDetails  (Client *client, ShowData &show, bool getEpisodeCountOnly, bool fetchPlaylist) const override;
    QList<VideoServer> loadServers  (Client *client, const PlaylistItem* episode) const override;
    PlayItem           extractSource(Client *client, VideoServer &server) override;
private:
    QMap<QString, QString> m_headers = {
        {"accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7"},
        {"accept-language", "en-GB,en;q=0.9,zh-CN;q=0.8,zh;q=0.7"},
        {"user-agent", "Mozilla/5.0 (Linux; Android 8.0.0; moto g(6) play Build/OPP27.91-87) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/69.0.3497.100 Mobile Safari/537.36" },
        // {"user-agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/131.0.0.0 Safari/537.36"},
        {"referer", hostUrl()}
    };
};



