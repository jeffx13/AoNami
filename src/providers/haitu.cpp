#include "haitu.h"
#include "network/csoup.h"

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
    QString url = baseUrl + (sortBy == "--" ? "vodsearch/": "vodshow/")
               + query + "--" + sortBy + "------" + QString::number(page) + "---.html";

    auto showNodes = client->get(url).toSoup()
        .select("//div[@class='module-list']/div[@class='module-items']/div");

    QList<ShowData> shows;
    for (const auto &node : showNodes)
    {
        auto moduleItemCover = node.selectFirst(".//div[@class='module-item-cover']");
        auto videoClass = moduleItemCover.selectFirst(".//span[@class='video-class']").text();
        if (videoClass == "伦理片") continue;

        auto img = moduleItemCover.selectFirst(".//div[@class='module-item-pic']/img");
        QString title = img.attr("alt");
        QString coverUrl = img.attr("data-src");
        if(coverUrl.startsWith ('/')) {
            coverUrl = baseUrl + coverUrl;
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
    auto doc = client->get(baseUrl + show.link).toSoup();
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
    if (serverNodes.empty()) throw MyException("No servers founds!");
    auto serverNamesNode = doc.select("//div[@class='module-heading']//div[@class='module-tab-content']/div");
    Q_ASSERT(serverNamesNode.size() == serverNodes.size());


    QMap<float, QString> episodesMap1;
    QMap<QString, QString> episodesMap2;

    int episodeCount = 1;
    if (getEpisodeCount) {
        int maxEpisodes = 0;
        for (int i = 0; i < serverNodes.size(); i++) {
            auto serverNode = serverNodes[i];
            QString serverName = serverNamesNode[i].attr("data-dropdown-value");
            auto episodeNodes = serverNode.select(".//a");
            if (episodeNodes.size() > maxEpisodes) {
                maxEpisodes = episodeNodes.size();
            }
        }
        episodeCount = maxEpisodes;
    }

    if (getPlaylist) {
        int maxEpisodes = 0;
        for (int i = 0; i < serverNodes.size(); i++) {
            auto serverNode = serverNodes[i];
            QString serverName = serverNamesNode[i].attr("data-dropdown-value");
            auto episodeNodes = serverNode.select(".//a");
            if (episodeNodes.size() > maxEpisodes) {
                maxEpisodes = episodeNodes.size();
            }
            episodeCount = maxEpisodes;
            for (const auto &episodeNode : episodeNodes) {
                QString title = episodeNode.selectFirst(".//span").text();
                QString link = episodeNode.attr("href");
                float number = resolveTitleNumber(title);

                if (number > -1){
                    if (!episodesMap1[number].isEmpty()) episodesMap1[number] += ";";
                    episodesMap1[number] +=  serverName + " " + link;
                } else {
                    episodesMap2[title] +=  serverName + " " + link;
                    // show.addEpisode(0, number, serverName + " " + link, title);
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

PlayInfo Haitu::extractSource(Client *client, const VideoServer &server) const
{
    PlayInfo playInfo;
    QString response = client->get(baseUrl + server.link).body;
    static QRegularExpression player_aaaa_regex{R"(player_aaaa=(\{.*?\})</script>)"};
    QRegularExpressionMatch match = player_aaaa_regex.match(response);

    if (match.hasMatch()) {
        auto source = QJsonDocument::fromJson(match.captured (1).toUtf8()).object()["url"].toString();
        qDebug() << source;
        playInfo.sources.emplaceBack(source);
    } else {
        qWarning() << "Haitu failed to extract m3u8";
    }
    return playInfo;

}
