#include "autoembed.h"

QList<ShowData> Autoembed::search(Client *client, const QString &query, int page, int type) {
    QString url = "https://api.themoviedb.org/3/search/multi";
    QMap<QString, QString> params {
        {"api_key", "453752deba3272cd109112cd41127fd8"},
        {"query", QUrl::toPercentEncoding(query)},
        {"page", QString::number(page)}
    };
    auto results = client->get(url, {}, params).toJsonObject()["results"].toArray();
    QList<ShowData> shows;

    for (const auto &item : results) {
        auto showItem = item.toObject();
        QString mediaType = showItem["media_type"].toString();
        int type = ShowData::NONE;
        if (mediaType == "tv")
            type = ShowData::TVSERIES;
        else continue;

        auto posterPath = showItem["poster_path"].toString();
        QString coverUrl = posterPath.isEmpty() ? "" : "https://image.tmdb.org/t/p/w500/" + posterPath;
        QString title = showItem["title"].toString();
        if (title.isEmpty()) title = showItem["name"].toString();

        QString link = mediaType + "/" + QString::number(showItem["id"].toInt());
        shows.emplaceBack(title, link, coverUrl, this, "", type);
    }
    return shows;
}

QList<ShowData> Autoembed::popular(Client *client, int page, int type) {
    QString url;
    if (type == ShowData::TVSERIES) {
        url = "https://api.themoviedb.org/3/tv/top_rated?api_key=453752deba3272cd109112cd41127fd8&language=en-US&page=" + QString::number(page);
    } else {
        url = "https://api.themoviedb.org/3/movie/top_rated?api_key=453752deba3272cd109112cd41127fd8&language=en-US&page=" + QString::number(page);
    }
    auto results = client->get(url).toJsonObject()["results"].toArray();
    QList<ShowData> shows;

    for (const auto &item : results) {
        auto showItem = item.toObject();
        QString mediaType = type == ShowData::TVSERIES ? "tv" : "movie";

        auto posterPath = showItem["poster_path"].toString();
        QString coverUrl = posterPath.isEmpty() ? "" : "https://image.tmdb.org/t/p/w500/" + posterPath;
        QString title = showItem["title"].toString();
        if (title.isEmpty()) title = showItem["name"].toString();

        QString link = mediaType + "/" + QString::number(showItem["id"].toInt());
        shows.emplaceBack(title, link, coverUrl, this, "", type);
    }
    return shows;

}

QList<ShowData> Autoembed::latest(Client *client, int page, int type) {
    QString url;
    if (type == ShowData::TVSERIES) {
        url = "https://api.themoviedb.org/3/tv/airing_today?api_key=453752deba3272cd109112cd41127fd8&language=en-US&page=" + QString::number(page);
    } else {
        url = "https://api.themoviedb.org/3/movie/popular?api_key=453752deba3272cd109112cd41127fd8&language=en-US&page=" + QString::number(page);
    }

    auto results = client->get(url).toJsonObject()["results"].toArray();
    QList<ShowData> shows;

    for (const auto &item : results) {
        auto showItem = item.toObject();
        QString mediaType = type == ShowData::TVSERIES ? "tv" : "movie";

        auto posterPath = showItem["poster_path"].toString();
        QString coverUrl = posterPath.isEmpty() ? "" : "https://image.tmdb.org/t/p/w500/" + posterPath;
        QString title = showItem["name"].toString();
        if (title.isEmpty()) title = showItem["title"].toString();

        QString link = mediaType + "/" + QString::number(showItem["id"].toInt());
        shows.emplaceBack(title, link, coverUrl, this, "", type);
    }
    return shows;


}

int Autoembed::loadDetails(Client *client, ShowData &show, bool getEpisodeCountOnly, bool fetchPlaylist) const {
    QString infoUrl = "https://api.themoviedb.org/3/" + show.link + "?api_key=453752deba3272cd109112cd41127fd8";
    auto info = client->get(infoUrl).toJsonObject();
    show.status = info["status"].toString();
    show.description = info["overview"].toString();
    auto mediaTypeId = show.link.split('/');

    if (show.type == ShowData::MOVIE) {
        if (getEpisodeCountOnly) return 1;
        if (!fetchPlaylist) return true;
        auto imdbId = info["imdb_id"].toString();
        auto link = "https://player.autoembed.cc/embed/movie/" + imdbId;
        show.addEpisode(0, 0, link, show.title);
        return true;

    } else if (show.type == ShowData::TVSERIES) {
        if (getEpisodeCountOnly) return info["number_of_episodes"].toInt();
        if (!fetchPlaylist) return true;
        int seasons = info["number_of_seasons"].toInt();
        for (int i = 1; i <= seasons; i++) {
            QString seasonUrl = QString("https://api.themoviedb.org/3/%1/season/%2?api_key=453752deba3272cd109112cd41127fd8").arg(show.link, QString::number(i));
            auto season = client->get(seasonUrl).toJsonObject();
            auto airDate = season["air_date"].toString();
            if (airDate.isEmpty()) continue;

            auto episodes = season["episodes"].toArray();
            for (const auto &episodeObj : episodes) {
                auto episode = episodeObj.toObject();
                QString title = episode["name"].toString();
                float number = episode["episode_number"].toDouble();
                QString id = QString::number(episode["id"].toInt());
                // QString link = QString::number(episode["id"].toInt());
                // get substring before first / of show.link

                QString link = QString("%1&id=%2/%3/%4").arg(mediaTypeId.first(), mediaTypeId.last(), QString::number(i), QString::number(number)); //"movie&id=tt1877830

                show.addEpisode(i, number, link, title);
            }
        }
    }

    return true;
}

QList<VideoServer> Autoembed::loadServers(Client *client, const PlaylistItem *episode) const {
    QList<VideoServer> servers;

    auto dataServers = client->get(episode->link).toSoup().select("//div[@class='dropdown-menu']/button");
    for (const auto &server : dataServers) {
        std::string link;
        QString serverName = server.text().trimmed();
        CryptoPP::StringSource ss(server.attr("data-server").toStdString(), true,
                                  new CryptoPP::Base64Decoder(
                                      new CryptoPP::StringSink(link)
                                      )
                                  );
        // rLog() << serverName << link;
        servers.emplaceBack(serverName, QString::fromStdString(link));
    }
    return servers;
}

PlayItem Autoembed::extractSource(Client *client, VideoServer &server) {
    auto host = server.link.section('/', 1, 1, QString::SectionSkipEmpty);;
    if (host.startsWith("vidsrc")) {
        auto imdbId = server.link.section('/', -1, -1);

    }
    return {};
    // s.replace("&", "/").replace("id=","/");
    // auto referer = "https://tom.autoembed.cc/" + s;


    // auto url = "https://tom.autoembed.cc/api/getVideoSource?type=" + server.link;
    // auto response = client->get(url, {{"referer", referer}}).toJsonObject();
    // auto masterPlaylist = response["videoSource"].toString();
    // QVector<Video> videos;
    // QRegularExpression regex(R"#(#EXT-X-STREAM-INF:.*?RESOLUTION=(\d+x\d+).*?\n(https?://[^\s]+))#");
    // QRegularExpressionMatchIterator it = regex.globalMatch(client->get(masterPlaylist, {{"referer", referer}}).body);

    // for (auto i = it; i.hasNext();) {
    //     auto match = i.next();
    //     auto res = match.captured(1);
    //     auto link = match.captured(2);
    //     videos.emplaceBack(link);
    //     videos.last().resolution = res;
    // }



    // QList<SubTrack> subs;
    // auto subtitles = response["subtitles"].toArray();
    // for (const auto &sub : subtitles) {
    //     auto subObj = sub.toObject();
    //     subs.emplaceBack(subObj["label"].toString(), QUrl::fromUserInput(subObj["file"].toString()));
    // }
    // return PlayItem{videos, subs};

}
