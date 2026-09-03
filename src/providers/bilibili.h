#pragma once
#include "providers/showprovider.h"

class Bilibili : public ShowProvider
{
public:
    explicit Bilibili(QObject *parent = nullptr);
    QString name() const override { return "哔哩哔哩"; }
    QString hostUrl() const override { return "https://www.bilibili.com/"; }
    QStringList availableTypes() const override {
        return {"国创", "番剧", "电影", "电视剧", "综艺", "纪录片"};
    }

    QList<ShowData>    search       (Client *client, const QString &query, int page, int typeIndex) override;
    QList<ShowData>    popular      (Client *client, int page, int typeIndex) override;
    QList<ShowData>    latest       (Client *client, int page, int typeIndex) override;
    QList<VideoServer> loadServers  (Client *client, const PlaylistItem *episode) const override;
    PlayInfo           extractSource(Client *client, VideoServer server) override;

private:
    int loadShow(Client *client, ShowData &show, LoadParts parts) const override;
    QList<ShowData> filterSearch(Client *client, int sortBy, int page, int typeIndex);

    // GET a Bilibili endpoint via the in-China relay (bilibili/proxy) to unlock geo-locked content.
    Client::Response apiGet(Client *client, const QString &url,
                            const QMap<QString, QString> &params = {}) const;

    static constexpr int kSeasonTypes[] = {
        4, // 国创
        1, // 番剧
        2, // 电影
        5, // 电视剧
        7, // 综艺
        3, // 纪录片
    };
    static constexpr ShowData::ShowType kShowTypes[] = {
        ShowData::Anime,        // 国创
        ShowData::Anime,        // 番剧
        ShowData::Movie,        // 电影
        ShowData::TvSeries,     // 电视剧
        ShowData::Variety,      // 综艺
        ShowData::Documentary,  // 纪录片
    };

    QMap<QString, QString> m_headers;
    QString m_proxyApi;
};
