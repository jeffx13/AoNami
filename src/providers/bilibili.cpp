#include "bilibili.h"

QList<ShowData> Bilibili::search(Client *client, const QString &query, int page, int type)
{
    auto url = QString("https://search.bilibili.com/bangumi?keyword=%1&from_source=webtop_search&spm_id_from=666.16&search_source=5")
    .arg(QUrl::toPercentEncoding(query));
    auto showList = client->get(url).toSoup().select("//div[@class='media-card']");
    QList<ShowData> shows;
    for (const auto &showNode : std::as_const(showList)) {
        QString title = showNode.selectFirst(".//div[@class='media-card-content-head-title']/a").text();
        auto mediaCardImage = showNode.selectFirst("./a");
        QString coverUrl = "https" + showNode.selectFirst("./picture/source").attr("srcset");
        QString link = mediaCardImage.attr("href");
        QString latestText = showNode.selectFirst(".//div[@class='media-card-content-head-text media-card-content-head-label']/span[3]").text();
        shows.emplaceBack(title, link, coverUrl, this, latestText);
    }

    return shows;
}

QList<ShowData> Bilibili::popular(Client *client, int page, int type)
{
    return filterSearch(client, 3, page, type);
}

QList<ShowData> Bilibili::latest(Client *client, int page, int type)
{
    return filterSearch(client, 0, page, type);
}

QList<ShowData> Bilibili::filterSearch(Client *client, int sortBy, int page, int type) {
    QMap<QString, QString> params {
        { "st", "4" },
        { "style_id", "-1" },
        { "season_version", "-1" },
        { "is_finish", "-1" },
        { "copyright", "-1" },
        { "season_status", "-1" },
        { "year", "-1" },
        { "order", QString::number(sortBy) },
        { "sort", "0" },
        { "page", QString::number(page) },
        { "season_type", QString::number(types[type]) },
        { "pagesize", "20" },
        { "type", "1" }
    };

    QString url = proxyApi.isEmpty() ? "https://api.bilibili.com/pgc/season/index/result" : proxyApi + "index";
    auto showList = client->get(url, headers, params)
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
        shows.emplaceBack(title, link, coverUrl, this, latestText);
    }

    return shows;
}

int Bilibili::loadDetails(Client *client, ShowData &show, bool getEpisodeCountOnly, bool fetchPlaylist) const
{
    auto mediaSeasonId = show.link.split(" ");
    auto episodeList = client->get("https://api.bilibili.com/pgc/view/web/ep/list?season_id=" + mediaSeasonId[1], headers)
                           .toJsonObject()["result"].toObject()["episodes"].toArray();

    if (getEpisodeCountOnly) {
        int episodeCount = 0;
        for (int i = 0; i < episodeList.size(); i++) {
            auto episode = episodeList[i].toObject();
            if (episode["badge"] != "预告")
                episodeCount++;
        }
    }

    if (fetchPlaylist) {
        for (int i = 0; i < episodeList.size(); i++) {
            auto episode = episodeList[i].toObject();
            if (episode["badge"] == "预告" && i != episodeList.size() - 1)
                continue;

            auto id = QString::number(episode["ep_id"].toInt());
            auto title = episode["title"].toString();
            auto longTitle = episode["long_title"].toString();
            if (episode["badge"] == "预告")
                longTitle = "(预告) " + longTitle;

            float number = title.toFloat();
            show.addEpisode(0, number, id, longTitle);
        }
    }

    auto url = QString("https://www.bilibili.com/bangumi/media/md%1").arg(mediaSeasonId[0]);
    auto response = client->get(url);
    auto doc = response.toSoup();
    if (!doc) return false;
    auto mediaInfo = doc.selectFirst("//div[@class='media-info']");
    auto mediaInfoTime = doc.select("//div[@class='media-info-time']/span");
    show.releaseDate = mediaInfoTime[0].text();
    show.updateTime = mediaInfoTime[1].text();
    show.views = doc.selectFirst("//span[@class='media-info-count-item media-info-count-item-play']/em").text();
    auto fanCount = doc.selectFirst("//span[@class='media-info-count-item media-info-count-item-fans']/em").text();
    show.status = QString("%1 人在追").arg(fanCount);

    auto mediaInfoScore = doc.selectFirst("//div[@class='media-info-score']");
    auto score = mediaInfoScore.selectFirst("./div[@class='media-info-score-content']").text();
    auto reviewTimes = mediaInfoScore.selectFirst(".//div[@class='media-info-review-times']").text();
    show.score = QString("%1 (%2)").arg(score, reviewTimes);

    auto genreNodes = doc.select("//span[@class='media-tag']");
    for (const auto &genreNode : std::as_const(genreNodes)) {
        show.genres += genreNode.text();
    }

    static QRegularExpression regex(R"("evaluate":"([^"]+))");
    auto evaluateMatch = regex.match(response.body);
    if (evaluateMatch.hasMatch()) {
        show.description = evaluateMatch.captured(1);
    }

    return true;
}

QList<VideoServer> Bilibili::loadServers(Client *client, const PlaylistItem *episode) const
{

    QJsonObject result;
    if (!proxyApi.isEmpty()) {
        result = client->get(proxyApi + "playurl?" + episode->link)
                     .toJsonObject()["result"].toObject();
    } else {
        result = client->get("https://api.bilibili.com/pgc/player/web/playurl?support_multi_audio=true&abtest=%7B%22pc_ogv_half_pay%22:%222%22%7D&qn=0&fnver=0&fnval=4048&fourk=1&gaia_source=&from_client=BROWSER&is_main_page=true&need_fragment=true&season_id="+episode->link+"&isGaiaAvoided=false&ep_id="+episode->link+"&voice_balance=1&drm_tech_type=2&area=" + episode->link, headers)
                     .toJsonObject()["result"].toObject();
    }

    auto dash = result["dash"].toObject();
    auto videos = dash["video"].toArray();
    auto audios = dash["audio"].toArray();
    QList<VideoServer> servers;
    // for (const auto &audio : audios) {
    //     auto audioObject = audio.toObject();
    //     auto bandwidth = audioObject["bandwidth"].toInt();
    //     auto audioBaseUrl = audios[0].toObject()["base_url"].toString();
    //     auto audioBackupUrl = audioObject["backup_url"].toArray()[0].toString();
    // }
    for (int i = 0; i < videos.size(); i++) {
        auto video = videos[i].toObject();
        auto height = video["height"].toInt();
        auto bandwidth = video["bandwidth"].toInt();
        // auto videoBaseUrl = video["base_url"].toString();
        auto videoBackupUrl = video["backup_url"].toArray()[0].toString();

        auto audioLink = audios[0].toObject()["backup_url"].toArray()[0].toString();; // 3 audios in total, use 2nd best
        auto link = QString("%1 %2").arg(videoBackupUrl, audioLink);

        servers.emplaceBack(QString("%1 (%2)").arg(QString::number(height), QString::number(bandwidth)), link);
    }

    return servers;
}

PlayInfo Bilibili::extractSource(Client *client, VideoServer &server)
{
    PlayInfo playInfo;
    auto videoAudio = server.link.split(" ");
    playInfo.sources.emplaceBack(videoAudio[0], videoAudio[1]);
    playInfo.sources[0].addHeader("referer", headers["referer"]);
    playInfo.sources[0].addHeader("user-agent", headers["user-agent"]);
    return playInfo;
}

// void Bilibili::loadFromHttp(){
//     auto response = client->get("https://www.bilibili.com/bangumi/play/ep" + episode->link, headers).body;
//     static QRegularExpression regex(R"(const playurlSSRData = ([.\s\S]*?)if\s*\(playurlSSRData\))");
//     QRegularExpressionMatch match = regex.match(response);

//     if (!match.hasMatch()) {
//         qWarning() << "playurlSSRData not found!";
//         return {};
//     }

//     QString jsonStr = match.captured(1).trimmed();

//     // Ensure trailing comma or semicolon are removed
//     if (jsonStr.endsWith(';')) jsonStr.chop(1);

//     // Parse JSON
//     QJsonParseError parseError;
//     QJsonDocument playurlSSRData = QJsonDocument::fromJson(jsonStr.toUtf8(), &parseError);

//     if (parseError.error != QJsonParseError::NoError || !playurlSSRData.isObject()) {
//         qWarning() << "JSON parsing failed:" << parseError.errorString();
//         return {};
//     }

//     auto playurlSSRDataObject = playurlSSRData.object();
//     auto videos = playurlSSRDataObject["result"].toObject()["video_info"].toObject()["dash"].toObject()["video"].toArray();
//     auto audios = playurlSSRDataObject["result"].toObject()["video_info"].toObject()["dash"].toObject()["audio"].toArray();

// }
