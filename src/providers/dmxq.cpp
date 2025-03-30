#include "dmxq.h"


QList<ShowData> Dmxq::search(Client *client, const QString &query, int page, int type)
{
    QString cleanedQuery = QUrl::toPercentEncoding (QString(query).replace (" ", "+"));
    // return filterSearch(client, cleanedQuery, "--", page);
    return {};
}

QList<ShowData> Dmxq::popular(Client *client, int page, int type)
{
    return filterSearch(client, type, "by_hits", page);
}

QList<ShowData> Dmxq::latest(Client *client, int page, int type)
{
    return filterSearch(client, type, "by_time", page);
}

QList<ShowData> Dmxq::filterSearch(Client *client, int type, const QString &sortBy, int page) {
    QString params = QString(R"({"type_id":"%1","sort":"%2","class":"类型","area":"地区","year":"年份","page":"%3","pageSize":"21","timestamp":"1742958903"})")
                         .arg(QString::number(typesMap[type]), sortBy, QString::number(page));

    auto list = invokeAPI(client, "screen/list", params)["data"].toObject()["list"].toArray();


    QList<ShowData> shows;
    for (const auto &it : list)
    {
        auto showJson = it.toObject();
        QString title = showJson["name"].toString();
        QString latestText = showJson["dynamic"].toString();
        QString cover = showJson["cover"].toString();
        QString id = showJson["id"].toString();
        shows.emplaceBack(title, id, cover, this, latestText);
    }

    return shows;
}

int Dmxq::loadDetails(Client *client, ShowData &show, bool getEpisodeCountOnly, bool fetchPlaylist) const
{
    QString params = QString(R"({"id":"%1","timestamp":"1743745255"})")
                         .arg(show.link);

    auto data = invokeAPI(client, "detail", params)["data"].toObject();
    if (data.isEmpty()) return false;
    auto servers = data["play_from"].toArray();
    if (getEpisodeCountOnly) return servers[0].toObject()["total"].toInt();

    show.releaseDate = data["year"].toString();
    show.score = data["score"].toString();
    show.description = data["content"].toString();
    auto genres = data["tags"].toArray();
    for (const auto &genre : genres) {
        show.genres += genre.toObject()["name"].toString();
    }

    if (!fetchPlaylist) return true;

    for (int i = 0; i < servers.count(); i++) {
        auto server = servers[i].toObject();
        if (i == 0) {
            auto list = server["list"].toArray();
            //rLog() << server["name"].toString() << server["code"].toString();
            for (const auto &episode : list) {
                auto episodeObject = episode.toObject();
                auto episodeName = episodeObject["episode_name"].toString();
                auto playUrl = episodeObject["play_url"].toString();
                resolveTitleNumber(episodeName);
                bool ok;
                auto number = episodeName.toFloat(&ok);
                if (ok) {
                    show.addEpisode(0, number, playUrl, "");
                } else {
                    show.addEpisode(0, -1, playUrl, episodeName);
                }
                // rLog() << episodeName << playUrl;
                // playlist->last()->servers = std::make_unique<QList<VideoServer>>();
                // playlist->last()->servers->emplaceBack(server["name"].toString(), server["code"].toString());
            }

        }

    }


    return true;
}

QList<VideoServer> Dmxq::loadServers(Client *client, const PlaylistItem *episode) const
{
    // auto serversString = episode->link.split (";");
    // QList<VideoServer> servers;
    // for (auto& serverString: serversString) {

    // }
    return {{"default", episode->link}};
}

PlayInfo Dmxq::extractSource(Client *client, VideoServer &server)
{
    PlayInfo playInfo;
    playInfo.sources.emplaceBack(server.link);


    return playInfo;

}
