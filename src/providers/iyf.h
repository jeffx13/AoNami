#include "showprovider.h"

#include <QJsonArray>
#include <QtConcurrent>
#pragma once

class IyfProvider: public ShowProvider
{
public:
    IyfProvider();
    std::string baseUrl = "https://www.iyf.tv";
    QString name() const override { return "爱壹帆"; }
    QList<int> getAvailableTypes() const override {
        return {ShowData::ANIME, ShowData::MOVIE, ShowData::TVSERIES, ShowData::VARIETY, ShowData::DOCUMENTARY};
    }

    QList<ShowData>          search       (Client *client, const QString &query, int page, int type) override;;
    QList<ShowData>          popular      (Client *client, int page, int type) override { return filterSearch (client, page, false, type); }
    QList<ShowData>          latest       (Client *client, int page, int type) override { return filterSearch (client, page, true, type); }
    bool                     loadDetails  (Client *client, ShowData &show, bool loadInfo, bool loadPlaylist) const override;
    QList<VideoServer>       loadServers  (Client *client, const PlaylistItem *episode) const override { return {VideoServer{"default", episode->link}}; };
    PlayInfo                 extractSource(Client *client, const VideoServer &server) const override;
private:
    QList<ShowData>          filterSearch (Client *client, int page, bool latest, int type);
    QJsonObject              invokeAPI    (Client *client, const QString &prefixUrl, const QString &params) const;
    QPair<QString, QString>& getKeys      (Client *client, bool update = false) const;
    QString                  hash         (const QString &input, const QPair<QString, QString> &keys) const;

    QMap<QString, QString> headers = {
        {"referer", "https://www.iyf.tv"},
        {"X-Requested-With", "XMLHttpRequest"}
    };
    QString expire = "1728137497.24687";
    QString sign = "e2645eb1db6700b8f870cc078f81b83dfd4bf0ba7b71f7c766bb664313f9d406_ae18e03626a41089b745f99ba805cbca";
    QString token = "7e3f3da80c4f4f76b36ffeec01385afd";
    QString uid = "128949566";

    QMap<int, QString> cid = {
        {ShowData::MOVIE,       "0,1,3"},
        {ShowData::TVSERIES,    "0,1,4"},
        {ShowData::VARIETY,     "0,1,5"},
        {ShowData::ANIME,       "0,1,6"},
        {ShowData::DOCUMENTARY, "0,1,7"}
    };

    // QMap<QString, ShowData::ShowType> types = {
    //     {"动漫", ShowData::ANIME},
    //     {"电视剧", ShowData::TVSERIES},
    //     {"电影", ShowData::MOVIE},
    //     {"综艺", ShowData::VARIETY},
    //     {"jilupian", ShowData::DOCUMENTARY}
    // };

};

