#include "showprovider.h"

#include <QJsonArray>
#include <QtConcurrent>
#pragma once

class IyfProvider: public ShowProvider
{
public:
    IyfProvider();
    std::string hostUrl = "https://www.iyf.tv";
    QString name() const override {
        return "爱壹帆";
    };
    QList<int> getAvailableTypes() const override {
        return {ShowData::ANIME, ShowData::MOVIE, ShowData::TVSERIES, ShowData::VARIETY, ShowData::DOCUMENTARY};
    };


    QList<ShowData> search(const QString &query, int page, int type) override;;

    QList<ShowData> filterSearch(int page, bool latest, int type);


    QList<ShowData> popular(int page, int type) override { return filterSearch (page, false, type); }
    QList<ShowData> latest(int page, int type) override { return filterSearch (page, true, type); }


    bool loadDetails(ShowData &show) const override;;
    int getTotalEpisodes(const QString &link) const override {return 0;};

    QList<VideoServer> loadServers(const PlaylistItem *episode) const override {
        return {VideoServer{"default", episode->link}};
    };

    QList<Video> extractSource(const VideoServer &server) const override;
private:
    QJsonObject invokeAPI(const QString &prefixUrl, const QString &params) const {
        auto url = prefixUrl + params + "&vv=" + hash(params) + "&pub=" + publicKey;
        return NetworkClient::get (url).toJson()["data"].toObject()["info"].toArray().at (0).toObject();
    }
    QString hash(const QString &input) const {
        auto toHash = publicKey + "&"  + input.toLower()+ "&"  + privateKey;
        QByteArray hash = QCryptographicHash::hash(toHash.toUtf8(), QCryptographicHash::Md5);
        return hash.toHex();
    }
    void updateKeys() {
        QString url("https://www.iyf.tv/list/anime?orderBy=0&desc=true");
        QRegularExpression pattern(R"("publicKey":"([^"]+)\","privateKey\":\[\"([^"]+)\")");
        QRegularExpressionMatch match = pattern.match(NetworkClient::get (url).body);
        // Perform the search
        if (!match.hasMatch() || match.lastCapturedIndex() != 2)
            throw MyException("Failed to update keys");

        publicKey = match.captured(1); // The first captured group (publicKey)
        privateKey = match.captured(2); // The second captured group (privateKey)

    }

    QMap<QString, QString> headers = {
        {"referer", "https://www.iyf.tv"},
        {"X-Requested-With", "XMLHttpRequest"}
    };
    QString expire = "1717531864.77701";
    QString sign = "a06c172a49c55d3bf20ac914c6f229b01ff3d276ea158c328c667f3a02ce75d2_ae18e03626a41089b745f99ba805cbca";
    QString token = "f34bc1e0522c49608b837d5029aeeb72";
    QString uid = "128949566";

    QString publicKey;
    QString privateKey;

    QMap<int, QString> cid = {
        {ShowData::MOVIE,       "0,1,3"},
        {ShowData::TVSERIES,    "0,1,4"},
        {ShowData::VARIETY,     "0,1,5"},
        {ShowData::ANIME,       "0,1,6"},
        {ShowData::DOCUMENTARY, "0,1,7"}
    };

    QMap<QString, ShowData::ShowType> types = {
        {"动漫", ShowData::ANIME},
        {"电视剧", ShowData::TVSERIES},
        {"电影", ShowData::MOVIE},
        {"综艺", ShowData::VARIETY},
        {"jilupian", ShowData::DOCUMENTARY}
    };

};

