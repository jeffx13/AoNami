#pragma once
#include <QDebug>
#include "showprovider.h"
#include <QDateTime>

class YingShi : public ShowProvider
{
public:
    explicit YingShi(QObject *parent = nullptr) : ShowProvider(parent) {
        for (auto it = typeMap.constBegin(); it != typeMap.constEnd(); ++it) {
            reverseTypeMap[it.value()] = it.key();
        }
    };
    QString name() const override { return "影视TV"; }
    QString hostUrl() const override { return "https://api.yingshi.tv/"; }
    QList<int> getAvailableTypes() const override { return typeMap.keys(); }

    QList<ShowData>    search       (Client *client, const QString &query, int page, int type) override {
        auto urlEncodedQuery = QUrl::toPercentEncoding(query);
        auto url = "https://api.yingshi.tv/vod/v1/search?wd="+urlEncodedQuery+"&limit=20&page=" + QString::number(page);
        return filterSearch(client, url, type);
    }
    QList<ShowData>    popular      (Client *client, int page, int type) override {
        auto url = "https://api.yingshi.tv/vod/v1/vod/list?order=desc&limit=30&tid=" + QString::number(typeMap[type]) +"&by=" + "hits_day" + "&page=" + QString::number(page);
        return filterSearch(client, url, type);
    }
    QList<ShowData>    latest       (Client *client, int page, int type) override {
        auto url = "https://api.yingshi.tv/vod/v1/vod/list?order=desc&limit=30&tid=" + QString::number(typeMap[type]) +"&by=" + "time" + "&page=" + QString::number(page);
        return filterSearch(client, url, type);

    }
    QList<ShowData>    filterSearch(Client *client, const QString &url, int type) {
        QList<ShowData> shows;
        // auto url = "https://api.yingshi.tv/vod/v1/vod/list?order=desc&limit=30&tid=" + QString::number(typeMap[type]) +"&by=" + sortBy + "&page=" + QString::number(page);
        static QMap<QString, QString> headerMap = {
            {"app-channel", "WEB"},
            {"app-name", "WEB"},
            {"app-version", ""},
            {"authorization", "Bearer null"},
            {"content-type", "application/json"},
            {"device-id", "application/json"},
            {"ip-address", "79.171.158.26"},
            {"origin", "https://www.yingshi.tv"},
            {"platform-os", "WEB"},
            {"priority", "u=1, i"}
        };
        auto data = client->get(url, headerMap).toJsonObject()["data"].toObject()["List"].toArray();
        for (const auto &item : data) {
            auto showItem = item.toObject();
            QString coverUrl = showItem["vod_pic"].toString();
            QString title = showItem["vod_name"].toString();
            QString link = QString::number(showItem["vod_id"].toInt());
            int typeId = showItem["type_id"].toInt();
            shows.emplaceBack(title, link, coverUrl, this, "", reverseTypeMap[typeId]);
        }
        return shows;
    }
    int                loadDetails  (Client *client, ShowData &show, bool loadInfo, bool getPlaylist, bool getEpisodeCount) const override;
    QList<VideoServer> loadServers  (Client *client, const PlaylistItem* episode) const override {
        auto serversString = episode->link.split (";");
        QList<VideoServer> servers;
        for (auto& serverString: serversString) {
            auto serverNameAndLink = serverString.split (" ");
            QString serverName = serverNameAndLink.first();
            QString serverLink = serverNameAndLink.last();
            servers.emplaceBack (serverName, serverLink);
        }
        return servers;
    }
    PlayInfo           extractSource(Client *client, VideoServer& server) const override {
        PlayInfo playInfo;
        playInfo.sources.emplaceBack(server.link);
        auto &video = playInfo.sources.first();
        video.addHeader("origin", "https://www.yingshi.tv");
        video.addHeader("user-agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/128.0.0.0 Safari/537.36");
        video.addHeader("accept-language", "en-GB,en;q=0.9,zh-CN;q=0.8,zh;q=0.7");
        video.addHeader("X-Forwarded-For", "127.0.0.1");

        // qDebug() << server.name << server.link;
        return playInfo;
    }
private:
    QMap<int, int> typeMap {
                           {ShowData::ANIME, 4},
                           {ShowData::MOVIE, 2},
                           {ShowData::TVSERIES, 1},
                           {ShowData::VARIETY, 3},
                           {ShowData::DOCUMENTARY, 5},
                           };
    QMap<int, int> reverseTypeMap;
};



