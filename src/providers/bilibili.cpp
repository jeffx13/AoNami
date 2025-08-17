#include "bilibili.h"
#include "app/config.h"
Bilibili::Bilibili(QObject *parent) : ShowProvider(parent) {
    auto config = Config::get();

    if (config.contains("bilibili_proxy")) {
        proxyApi = config["bilibili_proxy"].toString();
    }

    if (!config.contains("bilibili_cookies"))
        return;

    auto cookieJson = config["bilibili_cookies"].toObject();
    QStringList cookieList;
    for (auto it = cookieJson.begin(); it != cookieJson.end(); ++it) {
        cookieList << QString("%1=%2").arg(it.key(), it.value().toString());
    }

    QString cookieHeader = cookieList.join("; ");

    m_headers["cookie"] = cookieHeader;
}

QList<ShowData> Bilibili::search(Client *client, const QString &query, int page, int typeIndex)
{
    QString searchType = "media_ft";
    if (typeIndex == 0 || typeIndex == 1) {
        searchType = "media_bangumi";
    }


    QMap<QString, QString> params {
        { "category_id", "" },
        { "search_type", searchType },
        { "page", QString::number(page) },
        { "page_size", "20" },
        { "pubtime_begin_s", "0" },
        { "pubtime_end_s", "0" },
        { "from_spmid", "333.337" },
        { "platform", "pc" },
        { "highlight", "1" },
        { "single_column", "0" },
        { "keyword", QUrl::toPercentEncoding(query)},
        { "source_tag", "3" }
    };

    QJsonObject response;
    if (proxyApi.isEmpty()) {
        response =client->get("https://api.bilibili.com/x/web-interface/wbi/search/type", m_headers, params).toJsonObject();
    } else {
        response = client->get(proxyApi + "search", {}, params).toJsonObject();
    }
    auto results = response["data"].toObject()["result"].toArray();
    QList<ShowData> shows;
    for (int i = 0; i < results.size(); i++) {
        auto result = results[i].toObject();
        QString title = result["title"].toString();
        QString coverUrl = result["cover"].toString();
        QString seasonId = QString::number(result["season_id"].toInt());
        QString mediaId = QString::number(result["media_id"].toInt());
        QString link = QString("%1 %2").arg(mediaId, seasonId);
        QString latestText = result["index_show"].toString();
        rLog() << name() << title << coverUrl << link;
        shows.emplaceBack(title, link, coverUrl, this, latestText, m_typeIndexToType[typeIndex]);
    }

    return shows;
}

QList<ShowData> Bilibili::popular(Client *client, int page, int typeIndex)
{
    // type == anime or guochuang then order = 3
    int order = (typeIndex == 0 || typeIndex == 1) ? 3 : 2;
    return filterSearch(client, order, page, typeIndex);
}

QList<ShowData> Bilibili::latest(Client *client, int page, int typeIndex)
{
    return filterSearch(client, 0, page, typeIndex);
}

QList<ShowData> Bilibili::filterSearch(Client *client, int sortBy, int page, int typeIndex) {
    QMap<QString, QString> params {
        { "st", QString::number(types[typeIndex]) },
        { "style_id", "-1" },
        { "season_version", "-1" },
        { "is_finish", "-1" },
        { "copyright", "-1" },
        { "season_status", "-1" },
        { "year", "-1" },
        { "order", QString::number(sortBy) },
        { "sort", "0" },
        { "page", QString::number(page) },
        { "season_type", QString::number(types[typeIndex]) },
        { "pagesize", "20" },
        { "type", "1" }
    };

    QString url = proxyApi.isEmpty() ? "https://api.bilibili.com/pgc/season/index/result" : proxyApi + "index";
    auto showList = client->get(url, m_headers, params)
                        .toJsonObject()["data"].toObject()["list"].toArray();

    QList<ShowData> shows;
    for (int i = 0; i < showList.size(); i++) {
        auto showJson = showList[i].toObject();
        QString seasonId = QString::number(showJson["season_id"].toInt());
        QString mediaId = QString::number(showJson["media_id"].toInt());
        QString link = QString("%1 %2").arg(mediaId, seasonId);
        QString title =showJson["title"].toString();
        QString coverUrl =showJson["cover"].toString();
        QString latestText =showJson["index_show"].toString();
        shows.emplaceBack(title, link, coverUrl, this, latestText, m_typeIndexToType[typeIndex]);
    }

    return shows;
}

int Bilibili::loadShow(Client *client, ShowData &show, bool getEpisodeCountOnly, bool getPlaylist, bool getInfo) const
{
    auto mediaAndSeasonId = show.link.split(" ");
    auto response = client->get("https://www.biliplus.com/api/bangumi?season=" + mediaAndSeasonId[1], m_headers).toJsonObject();
    if (response["code"].toInt() != 0) {
        oLog() << name() << "Error fetching details:" << response["message"].toString();
        return false;
    }
    auto result = response["result"].toObject();
    auto episodeList = result["episodes"].toArray();

    if (getEpisodeCountOnly) {
        int episodeCount = 0;
        for (int i = 0; i < episodeList.size(); i++) {
            auto episode = episodeList[i].toObject();
            if (episode["badge"] != "预告")
                episodeCount++;
        }
        return episodeCount;
    }

    if (getPlaylist) {
        for (int i = 0; i < episodeList.size(); i++) {
            auto episode = episodeList[i].toObject();
            if (episode["badge"] == "预告" && i != episodeList.size() - 1)
                continue;
            auto ep_id = QString::number(episode["ep_id"].toInt());
            auto link = QString(R"(%1&%2)").arg(mediaAndSeasonId[1], ep_id);
            auto title = episode["title"].toString();
            auto longTitle = episode["long_title"].toString();

            if (episode["badge"] == "预告")
                longTitle = "(预告) " + longTitle;
            bool ok;
            float number = title.toFloat(&ok);
            if (ok) {
                show.addEpisode(0, number, link, longTitle);
            } else {
                show.addEpisode(0, -1, link, title);
            }
        }
    }

    if (!getInfo) return episodeList.size();

    show.releaseDate = result["publish"].toObject()["pub_time_show"].toString();
    show.updateTime = result["new_ep"].toObject()["desc"].toString();
    auto stat = result["stat"].toObject();
    show.views = QLocale::system().toString(stat["views"].toInteger());
    show.status = stat["follow_text"].toString();

    auto rating = result["rating"].toObject();
    show.score = QString("%1 (%2)").arg(QString::number(rating["score"].toDouble()), QLocale::system().toString(rating["count"].toInt()));

    auto styles = result["styles"].toArray();
    for (int i = 0; i < styles.size(); i++) {
        show.genres.append(styles[i].toString());
    }
    show.coverUrl = result["cover"].toString();
    show.description = result["evaluate"].toString();

    return episodeList.size();
}

QList<VideoServer> Bilibili::loadServers(Client *client, const PlaylistItem *episode) const
{
    return {{"Default", episode->link}};
}

PlayInfo Bilibili::extractSource(Client *client, VideoServer &server)
{
    QJsonObject videoInfo;
    if (!proxyApi.isEmpty()) {
        videoInfo = client->get(proxyApi + "playurl?" + server.link)
        .toJsonObject()["data"].toObject()["video_info"].toObject();
    } else {
        auto seasonAndEpId = server.link.split('&');
        auto data = QString(R"({"scene":"normal","video_index":{"bvid":null,"cid":null,"ogv_season_id":%1,"ogv_episode_id":%2},"video_param":{"qn":0},"player_param":{"fnver":0,"fnval":4048,"drm_tech_type":2},"exp_info":{"ogv_half_pay":true}})")
                        .arg(seasonAndEpId[0], seasonAndEpId[1]).toUtf8();
        auto headers = m_headers;
        headers["cookie"] = "buvid3=D1DE135F-BAF5-2A82-179C-05615F3D16DE34615infoc";
        headers["content-type"] = "application/json";
        videoInfo = client->post("https://api.bilibili.com/ogv/player/playview?csrf=ceb398a9b56d805668d0844b15e1f5e4", data, headers)
                          .toJsonObject()["data"].toObject()["video_info"].toObject();
    }
    PlayInfo playItem;

    if (videoInfo.contains("dash")) {
        auto dash = videoInfo["dash"].toObject();
        auto videos = dash["video"].toArray();
        auto audios = dash["audio"].toArray();

        for (int i = 0; i < audios.size(); i++) {
            auto audio = audios[i].toObject();
            auto bandwidth = audio["bandwidth"].toInt();
            // auto audioBaseUrl = audios[0].toObject()["base_url"].toString();
            auto audioBackupUrl = audio["backup_url"].toArray()[0].toString();
            playItem.audios.emplaceBack(audioBackupUrl, QString::number(bandwidth));
        }
        for (int i = 0; i < videos.size(); i++) {
            auto video = videos[i].toObject();
            auto height = video["height"].toInt();
            auto width = video["width"].toInt();
            auto bandwidth = video["bandwidth"].toInt();
            auto label = QString("%1x%2 (%3)").arg(QString::number(width), QString::number(height), QString::number(bandwidth));
            // auto videoBaseUrl = video["base_url"].toString();
            auto videoBackupUrl = video["backup_url"].toArray()[0].toString();
            playItem.videos.emplaceBack(videoBackupUrl, label, height, bandwidth);
        }
    } else if (videoInfo.contains("durls")) {
        auto durls = videoInfo["durls"].toArray();
        for (int i = 0; i < durls.size(); i++) {
            auto item = durls[i].toObject();
            auto durl = item["durl"].toArray()[0].toObject();
            auto quality = item["quality"].toInt();
            auto size = durl["size"].toInt();
            auto url = durl["url"].toString();
            // auto backupUrl = durl["backup_url"].toArray()[0].toString();
            playItem.videos.emplaceBack(url, QString("%1 (%2)").arg(quality).arg(size));
        }
    } else {
        return playItem;
    }

    playItem.addHeader("referer", m_headers["referer"]);
    playItem.addHeader("user-agent", m_headers["user-agent"]);
    return playItem;
}

