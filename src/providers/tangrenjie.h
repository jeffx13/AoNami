#pragma once
#include <QString>
#include "showprovider.h"
class Tangrenjie: public ShowProvider
{
public:
    explicit Tangrenjie(QObject *parent = nullptr) : ShowProvider(parent) {};
    QString name() const override {return "唐人街影院";}

    QList<QString> getAvailableTypes() const override {
        return {"动漫", "电影", "电视剧", "综艺"};
    };
    QString hostUrl() const override { return "https://www.chinatownfilm.com"; };
    QList<ShowData> search(Client *client, const QString &query, int page, int type) override;
    QList<ShowData> popular(Client *client, int page, int type) override;
    QList<ShowData> latest(Client *client, int page, int type) override;
    int loadDetails(Client *client, ShowData &show, bool getEpisodeCountOnly, bool fetchPlaylist) const override;
    QList<VideoServer> loadServers(Client *client, const PlaylistItem *episode) const override;
    PlayItem extractSource(Client *client, VideoServer &server) override;


private:
    QMap<QString, QString> m_headers = {
        {"accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7"},
        {"accept-language", "en-GB,en;q=0.9,zh-CN;q=0.8,zh;q=0.7"},
        {"user-agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/131.0.0.0 Safari/537.36"}
    };

    QList<int> types = {
        4, // 动漫
        1, // 电影
        2, // 电视剧
        3, // 综艺
    };
};



