#pragma once
#include "showprovider.h"
#include <QJsonArray>
#include "config.h"
class IyfProvider: public ShowProvider
{
public:
    explicit IyfProvider(QObject *parent = nullptr) : ShowProvider(parent) {
        auto config = Config::get();
        if (!config.contains("iyf_auth"))
            return;

        auto auth = config["iyf_auth"].toObject();
        expire = auth["expire"].toString();
        sign = auth["sign"].toString();
        token = auth["token"].toString();
        uid = auth["uid"].toString();
    };

    QString name() const override { return "爱壹帆"; }
    QString hostUrl() const override { return  "https://www.iyf.tv"; }
    QList<QString> getAvailableTypes() const override {
        return {"动漫", "电影", "电视剧", "综艺", "纪录片"};
    }
    QList<ShowData>          search       (Client *client, const QString &query, int page, int type) override;;
    QList<ShowData>          popular      (Client *client, int page, int typeIndex) override { return filterSearch (client, page, false, typeIndex); }
    QList<ShowData>          latest       (Client *client, int page, int typeIndex) override { return filterSearch (client, page, true, typeIndex); }
    int                      loadDetails  (Client *client, ShowData &show, bool getEpisodeCountOnly, bool getPlaylist, bool getInfo = true) const override;
    QList<VideoServer>       loadServers  (Client *client, const PlaylistItem *episode) const override { return {VideoServer{"Default", episode->link}}; };
    PlayItem                 extractSource(Client *client, VideoServer &server) override;
private:
    QList<ShowData>          filterSearch (Client *client, int page, bool latest, int typeIndex);
    QJsonObject              invokeAPI    (Client *client, const QString &prefixUrl, const QString &query) const;
    QPair<QString, QString>& getKeys      (Client *client, bool update = false) const;
    QString                  hash         (const QString &input, const QPair<QString, QString> &keys) const;
    // void getUserInfo(Client *client) const {
    //     QString params = QString("cinema=1&uid=%1&expire=%2&gid=1&sign=%3&token=%4").arg(uid, expire, sign, token);
    //     auto infoJson = invokeAPI(client, "https://m10.iyf.tv/v3/user/getuserinfo?", params);
    //     cLog() << infoJson;
    // }
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


    // old
    // QString expire = "1743815212.59166";
    // QString sign = "e143ab636009bc685dfc65b846f4a53487caef368bf85da2e7bff7b3ab0e8495_67ea4d55e40d2a1b20940a64f6ddadd0";
    // QString token = "82d95487853b4691a8fe93670f6441be";
    // QString uid = "129552859";

    // QString expire = "1746716714.02158";
    // QString sign = "069406f6fb5b8519936c69a361126dac2309d0eb862f69642c9afc1638eb51e0_6ad4835155879861a3a39b9513125b87";
    // QString token = "2002818bfa7b42d8bdf3d88d922b9a85";
    // QString uid = "129619503";

    // 30.4.2025
    QString expire;
    QString sign;
    QString token;
    QString uid;




};

