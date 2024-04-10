#include "haitu.h"
#include <pugixml/pugixml.hpp>
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
    QString url = hostUrl + (sortBy == "--" ? "vodsearch/": "vodshow/")
               + query + "--" + sortBy + "------" + QString::number(page) + "---.html";

    auto showNodes = NetworkClient::get(url).document().select("//div[@class='module-list']/div[@class='module-items']/div");
    QList<ShowData> shows;

    for (pugi::xpath_node_set::const_iterator it = showNodes.begin(); it != showNodes.end(); ++it)
    {
        auto img = it->node().select_node(".//div[@class='module-item-pic']/img").node();
        QString title = img.attribute("alt").as_string();
        QString coverUrl = img.attribute("data-src").as_string();
        if(coverUrl.startsWith ('/')) {
            coverUrl = hostUrl + coverUrl;
        }
        // qDebug() << title <<coverUrl;
        QString link = it->node().select_node (".//div[@class='module-item-pic']/a").node().attribute ("href").as_string();
        QString latestText;

        if (sortBy == "--"){
            latestText = it->node().select_node (".//a[@class='video-serial']").node().child_value();
        } else {
            latestText = it->node().select_node (".//div[@class='module-item-text']").node().child_value();
        }

        shows.emplaceBack(title, link, coverUrl, this, latestText);
    }

    return shows;
}

bool Haitu::loadDetails(ShowData &show) const
{
    auto doc = NetworkClient::get(hostUrl + show.link).document();

    auto infoItems = doc.select ("//div[@class='video-info-items']/div");
    if (infoItems.empty()) return false;

    show.releaseDate = infoItems[2].node().child_value();
    show.updateTime = infoItems[3].node().child_value();
    show.updateTime = show.updateTime.split ("，").first();
    show.status = infoItems[4].node().child_value();
    show.score = infoItems[5].node().select_node (".//font").node().child_value();
    show.description = QString(infoItems[7].node().select_node (".//span").node().child_value()).trimmed();
    auto genreNodes = doc.select ("//div[@class='tag-link']/a");
    for (pugi::xpath_node_set::const_iterator it = genreNodes.begin(); it != genreNodes.end(); ++it) {
        show.genres += it->node().child_value();
    }

    pugi::xpath_node_set serverNodes = doc.select ("//div[@class='scroll-content']");
    if (serverNodes.empty()) return false;

    pugi::xpath_node_set serverNamesNode = doc.select("//div[@class='module-heading']//div[@class='module-tab-content']/div");

    Q_ASSERT (serverNamesNode.size() == serverNodes.size());
    serverNodes.sort (true);
    serverNamesNode.sort (true);

    PlaylistItem *playlist = nullptr;
    QMap<float, QString> episodesMap;
    // bool makeEpisodesHash = show.type == 2 || show.type == 4;

    for (int i = 0; i < serverNodes.size(); i++) {
        pugi::xpath_node serverNode = serverNodes[i];
        QString serverName = serverNamesNode[i].node().attribute ("data-dropdown-value").as_string();
        //qDebug() << "serverName" << QString::fromStdString (serverName);
        pugi::xpath_node_set episodeNodes = serverNode.node().select_nodes (".//a");
        //qDebug() << "episodes" << episodeNodes.size();

        for (pugi::xpath_node_set::const_iterator it = episodeNodes.begin(); it != episodeNodes.end(); ++it)
        {
            QString title = QString::fromStdString (it->node().select_node(".//span").node().child_value());
            static auto replaceRegex = QRegularExpression("[第集话完结期]");
            title = title.replace (replaceRegex,"").trimmed();
            bool ok;
            float intTitle = title.toFloat (&ok);
            float number = -1;
            if (ok) {
                number = intTitle;
                title.clear();
            }
            QString link = it->node().attribute ("href").as_string();
            // qDebug() << "link" << QString::fromStdString (link);

            if (number > -1){
                if (!episodesMap[number].isEmpty()) episodesMap[number] += ";";
                episodesMap[number] +=  serverName + " " + link;
            } else {
                show.addEpisode(number, serverName + " " + link, title);
            }


        }
    }

    for (auto [number, link] : episodesMap.asKeyValueRange()) {
        show.addEpisode (number, link,"");
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

QList<Video> Haitu::extractSource(const VideoServer &server) const
{
    QString response = NetworkClient::get(hostUrl + server.link).body;
    QRegularExpressionMatch match = player_aaaa_regex.match(response);

    if (match.hasMatch()) {
        auto source = QJsonDocument::fromJson (match.captured (1).toUtf8()).object()["url"].toString();
        return { Video(source) };
    }
    qWarning() << "Haitu failed to extract m3u8";
    return {};

}
