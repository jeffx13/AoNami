#pragma once

#include "providers/showprovider.h"
#include "config.h"

class Bilibili : public ShowProvider
{
public:
    explicit Bilibili(QObject *parent = nullptr) : ShowProvider(parent) {
        auto config = Config::get();
        if (!config.contains("bilibili_cookies"))
            return;

        auto cookieJson = config["bilibili_cookies"].toObject();
        QStringList cookieList;
        for (auto it = cookieJson.begin(); it != cookieJson.end(); ++it) {
            cookieList << QString("%1=%2").arg(it.key(), it.value().toString());
        }

        QString cookieHeader = cookieList.join("; ");

        headers["cookie"] = cookieHeader;
    };

    QString name() const override { return "哔哩哔哩"; }
    QString hostUrl() const override { return "https://www.bilibili.com/"; }
    QList<int> getAvailableTypes() const override {
        return {ShowData::ANIME};
    };

    QList<ShowData>    search       (Client *client, const QString &query, int page, int type) override;
    QList<ShowData>    popular      (Client *client, int page, int type) override;
    QList<ShowData>    latest       (Client *client, int page, int type) override;
    int                loadDetails  (Client *client, ShowData &show, bool getEpisodeCountOnly, bool fetchPlaylist) const override;
    QList<VideoServer> loadServers  (Client *client, const PlaylistItem* episode) const override;
    PlayInfo           extractSource(Client *client, VideoServer &server) override;
private:
    QList<ShowData>    filterSearch (Client *client, int sortBy, int page, int type);
    QMap<int, int> typesMap = {
        {ShowData::ANIME, 4},
        {ShowData::MOVIE, 1},
        {ShowData::TVSERIES, 2},
        {ShowData::VARIETY, 3}
    };
    QMap<QString, QString> headers {
        {"referer", "https://www.bilibili.com/"},
        {"user-agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
                       "AppleWebKit/537.36 (KHTML, like Gecko) "
                       "Chrome/134.0.0.0 Safari/537.36"}
    };

};
