#include "haitu.h"
#include "network/csoup.h"

QList<ShowData> Haitu::search(const QString &query, int page, int type)
{
    QString cleanedQuery = QUrl::toPercentEncoding (QString(query).replace (" ", "+"));
    return filterSearch(cleanedQuery, "--", page);
}

QList<ShowData> Haitu::popular(int page, int type)
{
    return filterSearch(QString::number(type), "hits", page);
}

QList<ShowData> Haitu::latest(int page, int type)
{
    return filterSearch(QString::number(type), "time", page);
}

QList<ShowData> Haitu::filterSearch(const QString &query, const QString &sortBy, int page) {    
    QString url = baseUrl + (sortBy == "--" ? "vodsearch/": "vodshow/")
               + query + "--" + sortBy + "------" + QString::number(page) + "---.html";
    auto doc = CSoup::connect(url);
    auto showNodes = doc.select("//div[@class='module-list']/div[@class='module-items']/div");
    QList<ShowData> shows;

    for (const auto &node : showNodes)
    {
        auto img = node.selectFirst(".//div[@class='module-item-pic']/img");
        QString title = img.attr("alt");
        QString coverUrl = img.attr("data-src");
        if(coverUrl.startsWith ('/')) {
            coverUrl = baseUrl + coverUrl;
        }
        // qDebug() << title <<coverUrl;
        QString link = node.selectFirst(".//div[@class='module-item-pic']/a").attr("href");
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

bool Haitu::loadDetails(ShowData &show, bool getPlaylist) const
{
    auto doc = CSoup::connect(baseUrl + show.link);

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

    if (!getPlaylist) return true;
    auto serverNodes = doc.select ("//div[@class='scroll-content']");
    if (serverNodes.empty()) return false;

    auto serverNamesNode = doc.select("//div[@class='module-heading']//div[@class='module-tab-content']/div");

    Q_ASSERT (serverNamesNode.size() == serverNodes.size());
    // serverNodes.sort (true);
    // serverNamesNode.sort (true);

    PlaylistItem *playlist = nullptr;
    QMap<float, QString> episodesMap;
    // bool makeEpisodesHash = show.type == 2 || show.type == 4;

    for (int i = 0; i < serverNodes.size(); i++) {
        auto serverNode = serverNodes[i];
        QString serverName = serverNamesNode[i].attr("data-dropdown-value");
        //qDebug() << "serverName" << QString::fromStdString (serverName);
        auto episodeNodes = serverNode.select(".//a");
        //qDebug() << "episodes" << episodeNodes.size();

        for (const auto &episodeNode : episodeNodes)
        {
            QString title = episodeNode.selectFirst(".//span").text();
            static auto replaceRegex = QRegularExpression("[第集话完结期]");
            title = title.replace (replaceRegex,"").trimmed();
            bool ok;
            float intTitle = title.toFloat (&ok);
            float number = -1;
            if (ok) {
                number = intTitle;
                title.clear();
            }
            QString link = episodeNode.attr("href");
            // qDebug() << "link" << QString::fromStdString (link);

            if (number > -1){
                if (!episodesMap[number].isEmpty()) episodesMap[number] += ";";
                episodesMap[number] +=  serverName + " " + link;
            } else {
                show.addEpisode(0, number, serverName + " " + link, title);
            }


        }
    }

    for (auto [number, link] : episodesMap.asKeyValueRange()) {
        show.addEpisode(0, number, link,"");
    }

    return true;
}

QList<VideoServer> Haitu::loadServers(const PlaylistItem *episode) const
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

PlayInfo Haitu::extractSource(const VideoServer &server) const
{
    PlayInfo playInfo;
    QString response = Client::get(baseUrl + server.link).body;
    QRegularExpressionMatch match = player_aaaa_regex.match(response);

    if (match.hasMatch()) {
        auto source = QJsonDocument::fromJson (match.captured (1).toUtf8()).object()["url"].toString();
        playInfo.sources.emplaceBack(source);
    }
    qWarning() << "Haitu failed to extract m3u8";
    return playInfo;

}
