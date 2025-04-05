#pragma once

#include "providers/showprovider.h"
#include "config.h"

class Bilibili : public ShowProvider
{

public:
    explicit Bilibili(QObject *parent = nullptr) : ShowProvider(parent) {
        auto config = Config::get();

        if (config.contains("bilibili_proxy")) {
            proxyApi = config["bilibili_proxy"].toString();
        }

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
    QList<QString> getAvailableTypes() const override {
        return {"国创", "番剧", "电影", "电视剧", "综艺", "纪录片"};
    };

    QList<ShowData>    search       (Client *client, const QString &query, int page, int type) override;
    QList<ShowData>    popular      (Client *client, int page, int type) override;
    QList<ShowData>    latest       (Client *client, int page, int type) override;
    int                loadDetails  (Client *client, ShowData &show, bool getEpisodeCountOnly, bool fetchPlaylist) const override;
    QList<VideoServer> loadServers  (Client *client, const PlaylistItem* episode) const override;
    PlayItem           extractSource(Client *client, VideoServer &server) override;
private:
    QList<ShowData>    filterSearch (Client *client, int sortBy, int page, int type);
    QList<int> types = {
        4, // 国创
        1, // 番剧
        2, // 电影
        5, // 电视剧
        7, // 综艺
        3, // 纪录片
    };
    QMap<QString, QString> headers {
        {"referer", "https://www.bilibili.com/"},
        {"user-agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
                       "AppleWebKit/537.36 (KHTML, like Gecko) "
                       "Chrome/134.0.0.0 Safari/537.36"}
    };
    QString proxyApi;
};
