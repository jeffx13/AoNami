#pragma once
#include <QDebug>
#include "showprovider.h"
#include "network/csoup.h"
#include <QDateTime>

class YingShi : public ShowProvider
{
public:
    YingShi(){
        for (auto it = typeMap.constBegin(); it != typeMap.constEnd(); ++it) {
            reverseTypeMap[it.value()] = it.key();
        }
    };
    QString name() const override { return "影视TV"; }
    QString baseUrl = "https://api.yingshi.tv/";
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
    int                loadDetails  (Client *client, ShowData &show, bool loadInfo, bool loadPlaylist, bool getEpisodeCount) const override {
        auto url = "https://api.yingshi.tv/vod/v1/info?id=" + show.link + "&tid=" + QString::number(typeMap[show.type]);
        auto showItem = client->get(url).toJsonObject()["data"].toObject();
        if (showItem.isEmpty()) return false;
        if (loadInfo) {
            show.description = showItem["vod_content"].toString();
            show.status = showItem["vod_remarks"].toString();
            QDateTime timestamp = QDateTime::fromSecsSinceEpoch(showItem["vod_time"].toInt());
            show.updateTime = timestamp.toString("yyyy-MM-dd hh:mm:ss");
            show.releaseDate = QString::number(showItem["vod_year"].toInt());
            show.coverUrl = showItem["vod_pic"].toString();
        }

        if (!loadPlaylist) return true;

        QMap<float, QString> episodesMap1;
        QMap<QString, QString> episodesMap2;
        auto sources = showItem["vod_sources"].toArray();

        for (const auto &source : sources) {
            auto sourceObject = source.toObject();
            // auto sourceCode = sourceObject["source_code"].toString();
            auto sourceName = sourceObject["source_name"].toString();
            auto playlist = sourceObject["vod_play_list"].toObject()["urls"].toArray();
            for (const auto &item : playlist) {
                auto episode = item.toObject();
                // float episodeNumber = episode["nid"].toInt();
                auto episodeName = episode["name"].toString();
                auto episodeLink = episode["url"].toString();
                float number = resolveTitleNumber(episodeName);
                if (number > -1){
                    if (!episodesMap1[number].isEmpty()) episodesMap1[number] += ";";
                    episodesMap1[number] +=  sourceName + " " + episodeLink;
                } else {
                    if (!episodesMap2[episodeName].isEmpty()) episodesMap2[episodeName] += ";";
                    episodesMap2[episodeName] +=  sourceName + " " + episodeLink;
                }
            }
        }
        for (auto [number, link] : episodesMap1.asKeyValueRange()) {
            show.addEpisode(0, number, link, "");
        }
        for (auto [title, link] : episodesMap2.asKeyValueRange()) {
            show.addEpisode(0, -1, link, title);
        }

        return true;
    }
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
    PlayInfo           extractSource(Client *client, const VideoServer& server) const override {
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



