#include "allanime.h"
#include "network/net.h"

QList<ShowData> AllAnime::search(Client *client, const QString &query, int page, int type) {
    QString url = "https://api.allanime.day/api?variables={\"search\":{\"query\":\""
                  + QUrl::toPercentEncoding(query) + "\"},\"limit\":26,\"page\":"
                  + QString::number(page)
                  + ",\"translationType\":\"sub\",\"countryOrigin\":\"ALL\"}&extensions={\"persistedQuery\":{\"version\":1,\"sha256Hash\":\"06327bc10dd682e1ee7e07b6db9c16e9ad2fd56c1b769e47513128cd5c9fc77a\"}}";




    auto data = client->get(url, headers).toJsonObject()["data"].toObject();
    QJsonArray showsJsonArray = data["shows"].toObject()["edges"].toArray();
    return parseJsonArray(showsJsonArray);
}

QList<ShowData> AllAnime::popular(Client *client, int page, int type) {
    //size = 25
    QString url = "https://api.allanime.day/api?variables={%22type%22:%22anime%22,%22size%22:25,%22dateRange%22:7,%22page%22:"
               + QString::number(page)
               + ",%22allowAdult%22:true,%22allowUnknown%22:false}&extensions={%22persistedQuery%22:{%22version%22:1,%22sha256Hash%22:%221fc9651b0d4c3b9dfd2fa6e1d50b8f4d11ce37f988c23b8ee20f82159f7c1147%22}}";

    QJsonObject jsonResponse = client->get(url, headers).toJsonObject();
    QJsonArray showJsonArray = jsonResponse["data"].toObject()["queryPopular"].toObject()["recommendations"].toArray();
    return parseJsonArray(showJsonArray, true);
}

QList<ShowData> AllAnime::latest(Client *client, int page, int type) {
    QString url = "https://api.allanime.day/api?variables={%22search%22:{%22sortBy%22:%22Recent%22},%22limit%22:26,%22page%22:"
               + QString::number(page)
               +",%22translationType%22:%22sub%22,%22countryOrigin%22:%22JP%22}&extensions={%22persistedQuery%22:{%22version%22:1,%22sha256Hash%22:%2206327bc10dd682e1ee7e07b6db9c16e9ad2fd56c1b769e47513128cd5c9fc77a%22}}";


    auto data = client->get(url, headers).toJsonObject()["data"].toObject();
    auto showsJsonArray = data["shows"].toObject()["edges"].toArray();
    return parseJsonArray(showsJsonArray);
}

int AllAnime::loadDetails(Client *client, ShowData &show, bool loadInfo, bool loadPlaylist, bool getEpisodeCount) const {
    QString url = "https://api.allanime.day/api?variables={%22_id%22:%22"
               + show.link
               +"%22}&extensions={%22persistedQuery%22:{%22version%22:1,%22sha256Hash%22:%229d7439c90f203e534ca778c4901f9aa2d3ad42c06243ab2c5e6b79612af32028%22}}";
    auto jsonResponse = client->get(url, headers).toJsonObject()["data"].toObject()["show"].toObject();

    if (jsonResponse.isEmpty()) return false;

    if (loadInfo) {
        show.description =  jsonResponse["description"].toString();
        show.status = jsonResponse["status"].toString();
        show.views =  jsonResponse["pageStatus"].toObject()["views"].toString();

        QJsonValue scoreValue = jsonResponse["score"];
        if (!scoreValue.isUndefined() && !scoreValue.isNull()) {
            show.score = QString::number(scoreValue.toDouble(), 'f', 1) + " (MAL)";
        }

        QJsonValue averageScoreValue = jsonResponse["averageScore"];
        if (!averageScoreValue.isUndefined() && !averageScoreValue.isNull()) {
            if (!show.score.isEmpty()) show.score += "; ";
            show.score += QString::number(averageScoreValue.toInt()) + " (Anilist)";
        }

        QJsonArray genresArray = jsonResponse["genres"].toArray();
        for (const QJsonValue& genreValue : genresArray) {
            show.genres.push_back(genreValue.toString());
        }

        QJsonObject airedStart = jsonResponse["airedStart"].toObject();
        int day = airedStart["date"].toInt(69);
        int month = airedStart["month"].toInt(69) + 1; // Adjusting month from 0-based to 1-based indexing
        int year = airedStart["year"].toInt(69);
        QDate airedStartDate(year, month, day);
        if (airedStartDate.isValid()){
            // Convert dayOfWeek to a string representing the day
            show.releaseDate = airedStartDate.toString("MMMM d, yyyy");
            show.updateTime = airedStartDate.toString("Every dddd");
        }

        if (airedStart.contains("hour")) {
            int hour = airedStart["hour"].toInt();
            int minute = airedStart["minute"].toInt(0);
            show.updateTime += QString(" at %1:%2").arg(hour, 2, 10, QLatin1Char('0')).arg(minute, 2, 10, QLatin1Char('0'));
        }
    }

    if (!loadPlaylist) return true;

    QJsonArray episodesArray = jsonResponse["availableEpisodesDetail"].toObject()["sub"].toArray();
    if (getEpisodeCount) return episodesArray.size();
    for (int i = episodesArray.size() - 1; i >= 0; --i) {
        QString episodeString = episodesArray.at(i).toString();
        QString episodeUrl = QString("https://api.allanime.day/api?variables={\"showId\":\"%1\",\"translationType\":\"sub\",\"episodeString\":\"%2\"}&extensions={\"persistedQuery\":{\"version\":1,\"sha256Hash\":\"5f1a64b73793cc2234a389cf3a8f93ad82de7043017dd551f38f65b89daa65e0\"}}")
                                 .arg(show.link, episodeString);
        show.addEpisode(0, episodeString.toFloat(), episodeUrl, "");
    }

    return true;
}

QList<VideoServer> AllAnime::loadServers(Client *client, const PlaylistItem *episode) const {
    QJsonObject jsonResponse = client->get(episode->link, headers).toJsonObject();
    QList<VideoServer> servers;

    QJsonArray sourceUrls = jsonResponse["data"].toObject()["episode"].toObject()["sourceUrls"].toArray();
    for (const QJsonValue &value : sourceUrls) {
        QJsonObject server = value.toObject();
        QString name = server["sourceName"].toString();
        QString link = server["sourceUrl"].toString();
        servers.emplaceBack (name, link);
    }

    return servers;
}

PlayInfo AllAnime::extractSource(Client *client, const VideoServer &server) const {
    PlayInfo playInfo;
    static QString endPoint;
    if (endPoint.isEmpty())
        endPoint = client->get(baseUrl + "getVersion").toJsonObject()["episodeIframeHead"].toString();

    auto decryptedLink = decryptSource(server.link);
    if (decryptedLink.startsWith ("/apivtwo/")) {
        auto url = endPoint + decryptedLink.insert (14,".json");
        QJsonArray links = client->get(url, headers).toJsonObject()["links"].toArray();
        for (const QJsonValue& value : links) {
            QJsonObject linkObject = value.toObject();
            auto isDash = linkObject["dash"].toBool();
            if (!isDash) {
                QString source = linkObject["link"].toString();
                playInfo.sources.emplaceBack(source);
            }
        }
    } else if (decryptedLink.contains ("streaming.php")) {
        GogoCDN gogo;
        QString source = gogo.extract(client, decryptedLink);
        playInfo.sources.emplaceBack(source);
    }

    return playInfo;
}

QString AllAnime::decryptSource(const QString &input) const {
    if (input.startsWith("-")) {
        // Extract the part after the last '-'
        QString hexString = input.section('-', -1);
        QByteArray bytes;

        // Convert each pair of hex digits to a byte and append to bytes array
        for (int i = 0; i < hexString.length(); i += 2) {
            bool ok;
            QString hexByte = hexString.mid(i, 2);
            bytes.append(static_cast<char>(hexByte.toInt(&ok, 16) & 0xFF));
        }

        // XOR each byte with 56 and convert to char
        QString result;
        for (char byte : bytes) {
            result += QChar(static_cast<char>(byte ^ 56));
        }

        return result;
    } else {
        // If the input does not start with '-', return it unchanged
        return input;
    }
}

QList<ShowData> AllAnime::parseJsonArray(const QJsonArray &showsJsonArray, bool isPopular) {
    QList<ShowData> animes;
    for (const QJsonValue& value : showsJsonArray) {
        QJsonObject animeJson = isPopular ? value.toObject()["anyCard"].toObject() : value.toObject();
        QString title = animeJson.value("name").toString();
        QString link = animeJson.value("_id").toString();
        if (title.isEmpty() && link.isEmpty()) continue;

        QString coverUrl = animeJson["thumbnail"].toString();
        coverUrl.replace("https:/", "https://wp.youtube-anime.com");
        if (coverUrl.startsWith("images3"))
            coverUrl = "https://wp.youtube-anime.com/aln.youtube-anime.com/" + coverUrl;

        animes.emplaceBack(title, link, coverUrl, this, "", ShowData::ANIME);

    }
    return animes;
}


