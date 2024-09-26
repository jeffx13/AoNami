#include "wolong.h"


QList<ShowData> Wolong::search(Client *client, const QString &query, int page, int type)
{
    QList<ShowData> shows;
    QString url = baseUrl + "ac=videolist&wd=" + query + "&pg=" + QString::number (page);
    auto list = client->get(url).toJsonObject()["list"].toArray();

    for (const auto &item : list) {
        auto showItem = item.toObject();
        QString coverUrl = showItem["vod_pic"].toString();
        QString title = showItem["vod_name"].toString();
        QString link = QString::number(showItem["vod_id"].toInt());
        shows.emplaceBack(title, link, coverUrl, this, "", ShowData::ANIME);
    }

    return shows;
}

QList<ShowData> Wolong::popular(Client *client, int page, int type) {
    QList<ShowData> shows;
    return shows;
}

QList<ShowData> Wolong::latest(Client *client, int page, int type) {
    QList<ShowData> shows;

    QString url = "https://collect.wolongzy.cc/api.php/provide/vod/?ac=videolist&pg=" + QString::number(page) + "&t=" + QString::number(typeMap[type]);
    auto list = client->get(url).toJsonObject()["list"].toArray();

    for (const auto &item : list) {
        auto showItem = item.toObject();
        QString coverUrl = showItem["vod_pic"].toString();
        QString title = showItem["vod_name"].toString();
        QString link = QString::number(showItem["vod_id"].toInt());
        shows.emplaceBack(title, link, coverUrl, this, "", ShowData::ANIME);
    }
    return shows;
}

int Wolong::loadDetails(Client *client, ShowData &show, bool loadInfo, bool getPlaylist, bool getEpisodeCount) const
{
    auto showItem = client->get(baseUrl + "ac=videolist&ids=" + show.link)
                        .toJsonObject()["list"].toArray().first().toObject();
    if (showItem.isEmpty()) return false;

    if (loadInfo) {
        show.description = showItem["vod_content"].toString();
        show.status = showItem["vod_remarks"].toString();
        show.updateTime = showItem["vod_time"].toString();
        show.releaseDate = showItem["vod_pubdate"].toString();
        show.score = showItem["vod_score"].toString();
        show.views = QString::number(showItem["vod_hits_month"].toInt());
        auto genres = showItem["vod_class"].toString().split(",");
        for (const auto &genre : genres) {
            show.genres.push_back(genre);
        }
    }

    if (!getPlaylist && !getEpisodeCount) return true;
    auto playlist = showItem["vod_play_url"].toString().split('#');
    int episodeCount = playlist.size();

    if (getPlaylist) {
        for (const auto& videoItem : playlist) {
            // split video once by '$' to get title and link
            auto video = videoItem.split('$');
            static auto replaceRegex = QRegularExpression("[第集话完结期]");
            QString title = video.first();
            float number = resolveTitleNumber(title);
            QString link = video.last();
            show.addEpisode(0, number, link, title);
        }
    }

    return episodeCount;
}




QList<VideoServer> Wolong::loadServers(Client *client, const PlaylistItem *episode) const
{
    QList<VideoServer> servers;
    servers.emplaceBack ("default", episode->link);
    return servers;
}

PlayInfo Wolong::extractSource(Client *client, const VideoServer &server) const {
    PlayInfo playInfo;
    playInfo.sources.emplaceBack(server.link);
    return playInfo;
}
