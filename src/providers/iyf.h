#include "showprovider.h"

#include <QJsonArray>
#include <QtConcurrent>
#pragma once

class IyfProvider: public ShowProvider
{
public:
    explicit IyfProvider(QObject *parent = nullptr) : ShowProvider(parent) {};

    QString name() const override { return "爱壹帆"; }
    QString hostUrl() const override { return  "https://www.iyf.tv"; }
    QList<int> getAvailableTypes() const override {
        return {ShowData::ANIME, ShowData::MOVIE, ShowData::TVSERIES, ShowData::VARIETY, ShowData::DOCUMENTARY};
    }
    QList<ShowData>          search       (Client *client, const QString &query, int page, int type) override;;
    QList<ShowData>          popular      (Client *client, int page, int type) override { return filterSearch (client, page, false, type); }
    QList<ShowData>          latest       (Client *client, int page, int type) override { return filterSearch (client, page, true, type); }
    int                      loadDetails  (Client *client, ShowData &show, bool loadInfo, bool getPlaylist, bool getEpisodeCount) const override;
    QList<VideoServer>       loadServers  (Client *client, const PlaylistItem *episode) const override { return {VideoServer{"default", episode->link}}; };
    PlayInfo                 extractSource(Client *client, VideoServer &server) override;
private:
    QList<ShowData>          filterSearch (Client *client, int page, bool latest, int type);
    QJsonObject              invokeAPI    (Client *client, const QString &prefixUrl, const QString &query) const;
    QPair<QString, QString>& getKeys      (Client *client, bool update = false) const;
    QString                  hash         (const QString &input, const QPair<QString, QString> &keys) const;
    void getUserInfo(Client *client) const {
        QString params = QString("cinema=1&uid=%1&expire=%2&gid=1&sign=%3&token=%4").arg(uid, expire, sign, token);
        auto infoJson = invokeAPI(client, "https://m10.iyf.tv/v3/user/getuserinfo?", params);
        qDebug() << infoJson;
    }
    QMap<QString, QString> headers = {
        {"referer", "https://www.iyf.tv"},
        {"X-Requested-With", "XMLHttpRequest"}
    };
    QString expire = "1743815212.59166";
    QString sign = "e143ab636009bc685dfc65b846f4a53487caef368bf85da2e7bff7b3ab0e8495_67ea4d55e40d2a1b20940a64f6ddadd0";
    QString token = "82d95487853b4691a8fe93670f6441be";
    QString uid = "129552859";

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

