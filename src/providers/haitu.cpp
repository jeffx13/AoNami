#include "haitu.h"


#include <QTemporaryFile>

QList<ShowData> Haitu::search(Client *client, const QString &query, int page, int type)
{
    QString cleanedQuery = QUrl::toPercentEncoding (QString(query).replace (" ", "+"));
    return filterSearch(client, cleanedQuery, "--", page);
}

QList<ShowData> Haitu::popular(Client *client, int page, int type)
{
    return filterSearch(client, QString::number(typesMap[type]), "hits", page);
}

QList<ShowData> Haitu::latest(Client *client, int page, int type)
{
    return filterSearch(client, QString::number(typesMap[type]), "time", page);
}

QList<ShowData> Haitu::filterSearch(Client *client, const QString &query, const QString &sortBy, int page) {
    QString url = hostUrl() + (sortBy == "--" ? "vodsearch/": "vodshow/")
               + query + "--" + sortBy + "------" + QString::number(page) + "---.html";

    auto showNodes = client->get(url).toSoup()
        .select("//div[@class='module-list']/div[@class='module-items']/div");

    QList<ShowData> shows;
    for (const auto &node : showNodes)
    {
        auto moduleItemCover = node.selectFirst(".//div[@class='module-item-cover']");

        if (sortBy== "--" &&
            moduleItemCover.selectFirst(".//span[@class='video-class']").text()== "伦理片")
            continue;

        auto img = moduleItemCover.selectFirst(".//div[@class='module-item-pic']/img");
        QString title = img.attr("alt");
        QString coverUrl = img.attr("data-src");
        if(coverUrl.startsWith ('/')) {
            coverUrl = hostUrl() + coverUrl;
        }


        QString link = moduleItemCover.selectFirst(".//div[@class='module-item-pic']//a").attr("href");
        QString latestText;

        if (sortBy == "--"){
            latestText = node.selectFirst (".//a[@class='video-serial']").text();
        } else {
            latestText = node.selectFirst(".//div[@class='module-item-text']").text();
        }

        shows.emplaceBack(title, link, coverUrl, this, latestText);
    }

    return shows;
}

int Haitu::loadDetails(Client *client, ShowData &show, bool loadInfo, bool getPlaylist, bool getEpisodeCount) const
{
    auto doc = client->get(hostUrl() + show.link).toSoup();
    if (!doc) return false;

    if (loadInfo) {
        auto infoItems = doc.select ("//div[@class='video-info-items']/div");
        if (infoItems.empty()) return false;
        show.releaseDate = infoItems[2].text();
        show.updateTime = infoItems[3].text();
        show.updateTime = show.updateTime.split ("，").first();
        show.status = infoItems[4].text();
        show.score = infoItems[5].selectFirst(".//font").text();
        show.description = infoItems[7].selectFirst(".//span").text().trimmed();
        auto genreNodes = doc.select ("//div[@class='tag-link']/a");
        for (const auto &genreNode : genreNodes) {
            show.genres += genreNode.text();
        }
    }

    if (!getPlaylist && !getEpisodeCount) return true;
    auto serverNodes = doc.select ("//div[@class='scroll-content']");
    if (serverNodes.empty()) throw MyException("No servers founds!", name());
    auto serverNamesNode = doc.select("//div[@class='module-heading']//div[@class='module-tab-content']/div");
    Q_ASSERT(serverNamesNode.size() == serverNodes.size());


    int maxEpisodeCount = 0;
    if (getEpisodeCount) {
        for (int i = 0; i < serverNodes.size(); i++) {
            maxEpisodeCount = std::max(maxEpisodeCount, (int)serverNodes[i].select(".//a").size());
        }
    }

    if (getPlaylist) {
        QMap<QString, QString> episodesMap;
        QVector<QString> insertOrder;
        for (int i = 0; i < serverNodes.size(); i++) {
            auto serverNode = serverNodes[i];
            QString serverName = serverNamesNode[i].attr("data-dropdown-value");
            auto episodeNodes = serverNode.select(".//a");
            maxEpisodeCount = std::max(maxEpisodeCount, (int)episodeNodes.size());
            for (const auto &episodeNode : episodeNodes) {
                QString title = episodeNode.selectFirst(".//span/text()").text();
                resolveTitleNumber(title);
                QString link = episodeNode.attr("href");
                if (!episodesMap.contains(title)) insertOrder.append(title);
                if (!episodesMap[title].isEmpty()) episodesMap[title] += ";";
                episodesMap[title] +=  serverName + " " + link;
            }
        }

        for (const auto &title: insertOrder) {
            bool ok;
            auto number = title.toFloat(&ok);
            if (ok) {
                show.addEpisode(0, number, episodesMap[title], "");
            } else {
                show.addEpisode(0, -1, episodesMap[title], title);
            }
        }

    }

    return maxEpisodeCount;
}

QList<VideoServer> Haitu::loadServers(Client *client, const PlaylistItem *episode) const
{
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

PlayInfo Haitu::extractSource(Client *client, VideoServer &server)
{
    PlayInfo playInfo;
    QString response = client->get(hostUrl() + server.link).body;
    static QRegularExpression player_aaaa_regex{R"(player_aaaa=(\{.*?\})</script>)"};
    QRegularExpressionMatch match = player_aaaa_regex.match(response);

    if (match.hasMatch()) {
        QString link = QJsonDocument::fromJson(match.captured (1).toUtf8()).object()["url"].toString();
        auto m3u8 = client->get(link).body.split ('\n');
        if (m3u8[0].trimmed().compare("#EXTM3U") != 0) {
            qWarning() << "Haitu: broken server" << server.name;
            return playInfo;
        }
        QUrl source = link;
        // QString tempFileName = QDir::tempPath() + "/kyokou/" + QUuid::createUuid().toString(QUuid::WithoutBraces) + ".m3u8";
        // QFile tempFile = QFile(tempFileName);
        // QTextStream tempFileStream(&tempFile);
        // if (tempFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        //     for (int i = 0; i < m3u8.size(); i++) {
        //         auto line = m3u8[i];
        //         if (line.trimmed().compare("#EXT-X-DISCONTINUITY") == 0)
        //         {
        //             i+=2;
        //             continue;
        //         }
        //         tempFileStream << line << "\n";
        //     }
        //     tempFile.close();
        //     source = QUrl::fromLocalFile(tempFile.fileName());
        // } else {
        //     oLog() << "Haitu" << "Failed to create temp file";
        // }
        playInfo.sources.emplaceBack(source);
    } else {
        qWarning() << "Haitu failed to extract m3u8";
    }




    return playInfo;

}
