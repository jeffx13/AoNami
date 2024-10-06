#include "yingshi.h"


int YingShi::loadDetails(Client *client, ShowData &show, bool loadInfo, bool getPlaylist, bool getEpisodeCount) const {
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

    if (!getPlaylist && !getEpisodeCount) return true;

    QMap<float, QString> episodesMap1;
    QMap<QString, QString> episodesMap2;
    auto sources = showItem["vod_sources"].toArray();
    int episodeCount = 0;

    if (getEpisodeCount) {
        for (const auto &source : sources) {
            auto sourceObject = source.toObject();
            auto urlCount = sourceObject["vod_play_list"].toObject()["url_count"].toInt();
            if (urlCount > episodeCount)
                episodeCount = urlCount;
        }
    }

    if (getPlaylist) {
        for (const auto &source : sources) {
            auto sourceObject = source.toObject();
            // auto sourceCode = sourceObject["source_code"].toString();
            auto sourceName = sourceObject["source_name"].toString();
            auto playlist = sourceObject["vod_play_list"].toObject()["urls"].toArray();
            if (playlist.size() > episodeCount)
                episodeCount = playlist.size();

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
    }

    return episodeCount;
}
