#include "allanime.h"
#include "extractors/gogocdn.h"

#include <config.h>

QList<ShowData> AllAnime::search(Client *client, const QString &query, int page, int type) {
    QString variables = "{%22search%22:{%22query%22:%22"+ QUrl::toPercentEncoding(query) + "%22},%22limit%22:26,%22page%22:" + QString::number(page)
        + ",%22translationType%22:%22sub%22,%22countryOrigin%22:%22ALL%22}";
    QString extensions = "{%22persistedQuery%22:{%22version%22:1,%22sha256Hash%22:%2206327bc10dd682e1ee7e07b6db9c16e9ad2fd56c1b769e47513128cd5c9fc77a%22}}";
    QString url = QString("https://api.allanime.day/api?variables=%1&extensions=%2").arg(variables, extensions);
    auto data = client->get(url, m_headers).toJsonObject()["data"].toObject();
    QJsonArray showsJsonArray = data["shows"].toObject()["edges"].toArray();
    return parseJsonArray(showsJsonArray);
}

QList<ShowData> AllAnime::popular(Client *client, int page, int typeIndex) {
    QString url = "https://api.allanime.day/api?variables={%22type%22:%22anime%22,%22size%22:25,%22dateRange%22:7,%22page%22:"
                  + QString::number(page)
                  + ",%22allowAdult%22:true,%22allowUnknown%22:false}&extensions={%22persistedQuery%22:{%22version%22:1,%22sha256Hash%22:%221fc9651b0d4c3b9dfd2fa6e1d50b8f4d11ce37f988c23b8ee20f82159f7c1147%22}}";

    QJsonObject jsonResponse = client->get(url, m_headers).toJsonObject();
    QJsonArray showJsonArray = jsonResponse["data"].toObject()["queryPopular"].toObject()["recommendations"].toArray();
    return parseJsonArray(showJsonArray, true);
}

QList<ShowData> AllAnime::latest(Client *client, int page, int typeIndex) {
    QString url = "https://api.allanime.day/api?variables={%22search%22:{%22sortBy%22:%22Recent%22},%22limit%22:26,%22page%22:"
                  + QString::number(page)
                  +",%22translationType%22:%22sub%22,%22countryOrigin%22:%22JP%22}&extensions={%22persistedQuery%22:{%22version%22:1,%22sha256Hash%22:%2206327bc10dd682e1ee7e07b6db9c16e9ad2fd56c1b769e47513128cd5c9fc77a%22}}";
    auto data = client->get(url, m_headers).toJsonObject()["data"].toObject();
    auto showsJsonArray = data["shows"].toObject()["edges"].toArray();
    return parseJsonArray(showsJsonArray);
}

int AllAnime::loadDetails(Client *client, ShowData &show, bool getEpisodeCountOnly, bool fetchPlaylist) const {
    QString url = "https://api.allanime.day/api?variables={%22_id%22:%22"
                  + show.link
                  +"%22}&extensions={%22persistedQuery%22:{%22version%22:1,%22sha256Hash%22:%229d7439c90f203e534ca778c4901f9aa2d3ad42c06243ab2c5e6b79612af32028%22}}";
    QJsonObject jsonResponse = client->get(url, m_headers).toJsonObject()["data"].toObject()["show"].toObject();
    if (jsonResponse.isEmpty()) return false;

    QJsonArray episodesArray = jsonResponse["availableEpisodesDetail"].toObject()["sub"].toArray();

    if (getEpisodeCountOnly) return episodesArray.size();

    show.description =  jsonResponse["description"].toString();
    show.status = jsonResponse["status"].toString();
    show.views =  jsonResponse["pageStatus"].toObject()["views"].toString();

    show.coverUrl = getCoverImage(jsonResponse);

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
    for (const QJsonValue& genreValue : std::as_const(genresArray)) {
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

    if (!fetchPlaylist) return true;

    for (int i = episodesArray.size() - 1; i >= 0; --i) {
        QString episodeString = episodesArray.at(i).toString();
        QString variables = QString(R"({"showId":"%1","translationType":"sub","episodeString":"%2"})")
                                .arg(show.link, episodeString);
        show.addEpisode(0, episodeString.toFloat(), variables, "");
    }

    return true;
}

QList<VideoServer> AllAnime::loadServers(Client *client, const PlaylistItem *episode) const {
    QString link = "https://api.allanime.day/api?variables=" + QUrl::toPercentEncoding(episode->link)
    + "&extensions={%22persistedQuery%22:{%22version%22:1,%22sha256Hash%22:%225f1a64b73793cc2234a389cf3a8f93ad82de7043017dd551f38f65b89daa65e0%22}}";
    QJsonObject jsonResponse = client->get(link, m_headers).toJsonObject();
    QList<VideoServer> servers;

    QJsonArray sourceUrls = jsonResponse["data"].toObject()["episode"].toObject()["sourceUrls"].toArray();
    for (int i = 0; i < sourceUrls.size(); ++i) {
        QJsonObject server = sourceUrls.at(i).toObject();
        QString sourceName = server["sourceName"].toString();
        QString link = server["sourceUrl"].toString();
        servers.emplaceBack(sourceName, link);
        // gLog() << "Server:" << sourceName << "Link:" << link;
    }

    return servers;
}

PlayItem AllAnime::extractSource(Client *client, VideoServer &server) {
    PlayItem playItem;
    static QString endPoint;
    if (endPoint.isEmpty())
        endPoint = client->get(hostUrl() + "getVersion").toJsonObject()["episodeIframeHead"].toString();

    auto decryptedLink = decryptSource(server.link);

    if (server.name == "Mp4") {
        auto response = client->get(decryptedLink, m_headers).body;
        static QRegularExpression regex(R"(src: "([^"]+))");
        QRegularExpressionMatch match = regex.match(response);
        if (match.hasMatch()) {
            QString source = match.captured(1);
            playItem.videos.emplaceBack(source);
            playItem.addHeader("referer", "https://mp4upload.com/");
            playItem.addHeader("user-agent", m_headers["User-Agent"]);
        }
    }
    else if (server.name == "Sw") {
        // // find index of last /
        // int lastSlashIndex = decryptedLink.lastIndexOf('/');
        // QString lastPart = decryptedLink.mid(lastSlashIndex + 1);
        // QString newUrl = "https://hlsflex.com/e/" + lastPart;
        // auto response = client->get(newUrl).body;
        // static QRegularExpression regex = QRegularExpression(R"(eval\(([\S\s]*?'\)\)\)))");
        // auto match = regex.match(response);
        // if (match.hasMatch()) {
        //     QString packed = match.captured(1);
        //     QJSEngine engine;
        //     QJSValue result = engine.evaluate(packed);
        //     qDebug() << result.toString();
        //     auto regex2 = QRegularExpression(R"(sources:\[{file:"[^"]+")");
        //     auto match2 = regex2.match(result.toString());
        //     if (match2.hasMatch()) {
        //         QString source = match2.captured(1);
        //         playItem.sources.emplaceBack(source);
        //         playItem.sources[0].referer = "https://swatchseries.to/";
        //         playItem.sources[0].userAgent = m_headers["User-Agent"];
        //     }
        // }
    }
    else if (server.name.startsWith("Yt")) {
        playItem.videos.emplaceBack(decryptedLink);
        playItem.addHeader("referer", "https://allanime.day/");
    }
    else if (decryptedLink.startsWith("/apivtwo")) {
        auto url = endPoint + decryptedLink.insert (14,".json");
        auto response = client->get(url, m_headers).toJsonObject();
        auto links = response["links"].toArray();

        for (int i = 0; i < links.size(); ++i) {
            QJsonObject link = links.at(i).toObject();

            if (link.contains("dash")) {
                auto rawUrls = link["rawUrls"].toObject();
                auto videos = rawUrls["vids"].toArray();
                auto audios = rawUrls["audios"].toArray();
                for (int i = 0; i < videos.size(); ++i) {
                    QJsonObject videoObject = videos[i].toObject();
                    int width = videoObject["width"].toInt();
                    int height = videoObject["height"].toInt();
                    int bandwidth = videoObject["bandwidth"].toInt();
                    QString label = QString("%1x%2 (%3)").arg(width).arg(height).arg(bytesIntoHumanReadable(bandwidth));
                    QString videoUrl = videoObject["url"].toString();
                    playItem.videos.emplaceBack(videoUrl, label, height, bandwidth);
                }
                for (int i = 0; i < audios.size(); ++i) {
                    QJsonObject audioObject = audios[i].toObject();
                    QString audioUrl = audioObject["url"].toString();
                    qint64 bandwidth = audioObject["bandwidth"].toInteger();
                    QString label = bytesIntoHumanReadable(bandwidth);
                    playItem.audios.emplaceBack(audioUrl, label);
                }
            } else {
                playItem.videos.emplaceBack(link["link"].toString());
            }


            if (link.contains("subtitles")) {
                auto subtitles = link["subtitles"].toArray();
                for (int i = 0; i < subtitles.size(); ++i) {
                    QJsonObject subObject = subtitles[i].toObject();
                    QString subUrl = subObject["src"].toString();
                    QString label = subObject["label"].toString();
                    if (subUrl.startsWith("https://allanime.pro/apiak/sk.json")) {
                        auto subtitleResponse = client->get(subUrl);

                        if (subtitleResponse.body.startsWith("{\"font")) {
                            auto subtitleJson = subtitleResponse.toJsonObject();
                            auto fileName = subUrl.split("?").last();

                            QString tmpDir = Config::getTempDir();
                            QString filePath = tmpDir + "/" + fileName;
                            QFile outputFile(filePath);

                            if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                                qDebug() << "Failed to open output file!";
                            }

                            QTextStream out(&outputFile);

                            int index = 1;
                            auto subtitleBody = subtitleJson["body"].toArray();
                            for (int j = 0; j < subtitleBody.size(); ++j) {
                                auto line = subtitleBody.at(j).toObject();
                                double from = line["from"].toDouble();
                                double to = line["to"].toDouble();
                                QString content = line["content"].toString().replace("\n", "\n");

                                // Write subtitle block
                                out << index << "\n";
                                out << msToSrtTime(from) << " --> " << msToSrtTime(to) << "\n";
                                out << content << "\n\n";

                                index++;
                            }
                            outputFile.close();
                            subUrl = QFileInfo(outputFile).absoluteFilePath();
                        }
                    }
                    playItem.subtitles.emplaceBack(subUrl, label);
                }
            }


        }
    } else if (decryptedLink.contains("streaming.php")) {
        GogoCDN gogo;
        //TODO fix gogo
        QString source = gogo.extract(client, decryptedLink);
        if (!source.isEmpty()){
            // rLog() << "all" << source;
            playItem.videos.emplaceBack(source);
        }

    }
    // rLog() << "all" << playItem.sources.count() << playItem.sources.first().videoUrl;
    return playItem;
}

QString AllAnime::getCoverImage(const QJsonObject &jsonResponse) const {
    QString coverUrl = jsonResponse["thumbnail"].toString();
    if (coverUrl.startsWith("https"))
        coverUrl.replace("https:/", "https://wp.youtube-anime.com");
    else
        coverUrl = "https://wp.youtube-anime.com/aln.youtube-anime.com/" + coverUrl;
    coverUrl += "?w=250";
    return coverUrl;
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
        return input;
    }
}

QList<ShowData> AllAnime::parseJsonArray(const QJsonArray &showsJsonArray, bool isPopular) {
    QList<ShowData> animes;
    for (int i = 0; i < showsJsonArray.size(); ++i) {
        QJsonObject anime = showsJsonArray.at(i).toObject();
        if (anime.isEmpty())
            continue;
        if (isPopular)
            anime = anime["anyCard"].toObject();

        QString title = anime.value("name").toString();
        QString link = anime.value("_id").toString();
        if (title.isEmpty() && link.isEmpty())
            continue;

        QString coverUrl = getCoverImage(anime);
        animes.emplaceBack(title, link, coverUrl, this, "", ShowData::ANIME);

    }
    return animes;
}


