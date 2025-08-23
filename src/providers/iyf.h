#pragma once
#include "showprovider.h"
#include <QJsonArray>
class Iyf: public ShowProvider
{
public:
    explicit Iyf(QObject *parent = nullptr);;

    QString name() const override { return "爱壹帆"; }
    QString hostUrl() const override { return  "https://www.iyf.tv"; }
    QList<QString> getAvailableTypes() const override {
        return {"动漫", "电影", "电视剧", "综艺", "纪录片"};
    }
    QList<ShowData>          search       (Client *client, const QString &query, int page, int type) override;;
    QList<ShowData>          popular      (Client *client, int page, int typeIndex) override { return filterSearch (client, page, false, typeIndex); }
    QList<ShowData>          latest       (Client *client, int page, int typeIndex) override { return filterSearch (client, page, true, typeIndex); }
    QList<VideoServer>       loadServers  (Client *client, const PlaylistItem *episode) const override { return {VideoServer{"Default", episode->link}}; };
    PlayInfo                 extractSource(Client *client, VideoServer &server) override;
private:
    QList<ShowData>          filterSearch (Client *client, int page, bool latest, int typeIndex);
    QJsonObject              invokeAPI    (Client *client, const QString &prefixUrl, const QString &query) const;
    QPair<QString, QString>& getKeys      (Client *client, bool update = false) const;
    QString                  hash         (const QString &input, const QPair<QString, QString> &keys) const;
    int                      loadShow  (Client *client, ShowData &show, bool getEpisodeCountOnly, bool getPlaylist, bool getInfo = true) const override;
    
    QMap<QString, QString> headers = {
        {"referer", "https://www.iyf.tv"},
        {"X-Requested-With", "XMLHttpRequest"}
    };
    QList<QString> cid = {
        "0,1,6" , // 动漫
        "0,1,3" , // 电影
        "0,1,4" , // 电视剧
        "0,1,5" , // 综艺
        "0,1,7" , // 纪录片
    };
    QList<ShowData::ShowType> m_typeIndexToType = {
        ShowData::ANIME, // 动漫
        ShowData::MOVIE, // 电影
        ShowData::TVSERIES, // 电视剧
        ShowData::VARIETY, // 综艺
        ShowData::DOCUMENTARY, // 纪录片
    };

    QString expire;
    QString sign;
    QString token;
    QString uid;




};

