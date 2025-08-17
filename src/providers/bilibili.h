#pragma once

#include "providers/showprovider.h"


class Bilibili : public ShowProvider
{

public:
    explicit Bilibili(QObject *parent = nullptr);;

    QString name() const override { return "哔哩哔哩"; }
    QString hostUrl() const override { return "https://www.bilibili.com/"; }
    QList<QString> getAvailableTypes() const override {
        return {"国创", "番剧", "电影", "电视剧", "综艺", "纪录片"};
    };

    QList<ShowData>    search       (Client *client, const QString &query, int page, int typeIndex) override;
    QList<ShowData>    popular      (Client *client, int page, int typeIndex) override;
    QList<ShowData>    latest       (Client *client, int page, int typeIndex) override;
    QList<VideoServer> loadServers  (Client *client, const PlaylistItem* episode) const override;
    PlayInfo           extractSource(Client *client, VideoServer &server) override;
private:
    int                loadShow  (Client *client, ShowData &show, bool getEpisodeCountOnly, bool getPlaylist, bool getInfo = true) const override;
    QList<ShowData>    filterSearch (Client *client, int sortBy, int page, int typeIndex);
    QList<int> types = {
        4, // 国创
        1, // 番剧
        2, // 电影
        5, // 电视剧
        7, // 综艺
        3, // 纪录片
    };
    QList<ShowData::ShowType> m_typeIndexToType {
        ShowData::ANIME,
        ShowData::ANIME,
        ShowData::MOVIE,
        ShowData::TVSERIES,
        ShowData::VARIETY,
        ShowData::DOCUMENTARY
    };

    QMap<QString, QString> m_headers {
        {"referer", "https://www.bilibili.com/"},
        {"user-agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
                       "AppleWebKit/537.36 (KHTML, like Gecko) "
                       "Chrome/134.0.0.0 Safari/537.36"}
    };
    QString proxyApi;
};
