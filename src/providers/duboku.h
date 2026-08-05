#pragma once
#include "showprovider.h"

class Duboku : public ShowProvider {
public:
    explicit Duboku(QObject *parent = nullptr) : ShowProvider(parent) {}
    QString name() const override { return "Duboku"; }
    QString hostUrl() const override { return "https://www.duboku.lv/"; }

    QList<QString>     getAvailableTypes() const override { return {"动漫", "电影", "剧集", "综艺"}; }
    QList<ShowData>    search       (Client *client, const QString &query, int page, int type) override;
    QList<ShowData>    popular      (Client *client, int page, int typeIndex) override;
    QList<ShowData>    latest       (Client *client, int page, int typeIndex) override;
    QList<VideoServer> loadServers  (Client *client, const PlaylistItem *episode) const override;
    PlayInfo           extractSource(Client *client, VideoServer server) override;

private:
    int loadShow(Client *client, ShowData &show, bool getEpisodeCountOnly, bool getPlaylist, bool getInfo) const override;

    QList<ShowData> parseList(const QString &html, int showType);
    QList<ShowData> listing(Client *client, int page, int typeIndex, const QString &by);
    QString absolute(const QString &path) const;

    static int showTypeOf(const QString &label);

    static constexpr int kTypeIds[] = {4, 1, 2, 3};

    QMap<QString, QString> m_headers = {
        {"User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:143.0) Gecko/20100101 Firefox/143.0"},
        {"Referer",    "https://www.duboku.lv/"},
    };
};
