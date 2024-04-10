
#include "kimcartoon.h"

#include <QTextDocument>

QVector<ShowData> Kimcartoon::search(const QString &query, int page, int type) {

    if (page > 1)
        return {};
    QString url = hostUrl + "Search/Cartoon";
    pugi::xpath_node_set showNodes = NetworkClient::post(url, {}, {{"keyword", query}})
                                         .document()
                                         .select("//div[@class='list-cartoon']/div/a[1]");
    return parseResults (showNodes);
}

QVector<ShowData> Kimcartoon::popular(int page, int type) {
    QString url = hostUrl + "CartoonList/MostPopular" + "?page=" + QString::number(page);
    return filterSearch(url);
}

QVector<ShowData> Kimcartoon::latest(int page, int type) {
    QString url = hostUrl + "CartoonList/LatestUpdate" + "?page=" + QString::number(page);
    return filterSearch(url);
}

QVector<ShowData> Kimcartoon::filterSearch(const QString &url) {
    auto showNodes = NetworkClient::get(url).document().select("//div[@class='list-cartoon']/div/a[1]");
    if (showNodes.empty())
        return {};
    return parseResults (showNodes);
}

bool Kimcartoon::loadDetails(ShowData &show) const {
    auto doc = NetworkClient::get(hostUrl + show.link).document();
    auto infoDiv = doc.selectFirst("//div[@class='barContent']").node();

    if (infoDiv.empty()) return false;

    show.title = infoDiv.select_node (".//a[@class='bigChar']").node().child_value();
    show.title.replace ("\n", " ");

    if (pugi::xml_node descriptionParagraph
        = infoDiv.select_node(".//span[contains(text() ,'Summary')]/parent::p/following-sibling::p[1]").node())
        show.description = QString(descriptionParagraph.child_value()).replace ('\n'," ").replace ("&nbsp"," ").trimmed();


    if (pugi::xml_node dateAiredTextNode
        = doc.selectFirst ("//span[contains(text() ,'Date')]/following-sibling::text()").node();
        dateAiredTextNode.type() == pugi::node_pcdata) {
        show.releaseDate = dateAiredTextNode.value();
    }

    if (pugi::xml_node statusTextNode =
        infoDiv.select_node (".//span[contains(text() ,'Status')]/following-sibling::text()[1]").node();
        statusTextNode.type() == pugi::node_pcdata)
        show.status = statusTextNode.value();

    if (pugi::xml_node viewsTextNode =
        infoDiv.select_node (".//span[contains(text() ,'Views')]/following-sibling::text()[1]").node();
        viewsTextNode.type() == pugi::node_pcdata)
        show.views = viewsTextNode.value();

    pugi::xpath_node_set genreNodes = infoDiv.select_nodes ("//span[contains(text(),'Genres')]/following-sibling::a");
    for (pugi::xpath_node_set::const_iterator it = genreNodes.begin(); it != genreNodes.end(); ++it) {
        QString genre = it->node().child_value();
        show.genres.push_back(genre.trimmed());
    }
    QRegularExpression titleRegex(QString(show.title).replace (" ", "\\s*"));

    pugi::xpath_node_set episodeNodes = doc.select("//table[@class='listing']//a");

    for (int i = episodeNodes.size() - 1; i >= 0; --i) {
        const pugi::xpath_node *it = &episodeNodes[i];
        QStringList fullEpisodeName;
        auto episodeNameString = QString(it->node().child_value()).replace (titleRegex, "").replace ("\n", "").trimmed();
        if (episodeNameString.startsWith ("Episode")){

            episodeNameString.remove (0, 8);
            if (int delimiterIndex = episodeNameString.indexOf(" - "); delimiterIndex != -1) {
                fullEpisodeName << episodeNameString.left(delimiterIndex);
                fullEpisodeName << episodeNameString.mid(delimiterIndex + 3);
            } else if (int spaceIndex = episodeNameString.indexOf(" "); spaceIndex != -1){
                fullEpisodeName << episodeNameString.left(spaceIndex);
                fullEpisodeName << episodeNameString.mid(spaceIndex + 1);
            } else {
                fullEpisodeName << episodeNameString;
            }
        } else {
            fullEpisodeName << episodeNameString;
        }


        QString title;
        bool ok;
        QString link;
        int number = -1;

        if (fullEpisodeName.size() == 2) {
            if (int intTitle = fullEpisodeName.first().toInt (&ok); ok)
                number = intTitle;
            title = fullEpisodeName.last();
        } else if (fullEpisodeName.size() == 1){

            if (int intTitle = fullEpisodeName.first().toInt (&ok); ok)
                number = intTitle;
            else title = fullEpisodeName.first();
        }

        link = it->node().attribute("href").value();
        show.addEpisode(number, link, title);
    }


    return true;
}

QVector<VideoServer>
Kimcartoon::loadServers(const PlaylistItem *episode) const {
    auto doc = NetworkClient::get(hostUrl + episode->link).document();
    auto serverNodes = doc.select("//select[@id='selectServer']/option");
    QList<VideoServer> servers;
    for (pugi::xpath_node_set::const_iterator it = serverNodes.begin(); it != serverNodes.end(); ++it) {
        QString serverName = QString(it->node().child_value()).trimmed();
        QString serverLink = it->node().attribute ("value").as_string();
        servers.emplaceBack (serverName, serverLink);
    }
    return servers;
}
QList<Video> Kimcartoon::extractSource(const VideoServer &server) const {
    auto doc = NetworkClient::get(hostUrl + server.link).document();
    auto iframe = doc.select ("//iframe[@id='my_video_1']");
    if (iframe.empty()) return {};
    QString serverUrl = iframe.first().node().attribute ("src").as_string();
    Functions::httpsIfy (serverUrl);
    auto response = NetworkClient::get(serverUrl, {{"sec-fetch-dest", "iframe"}}).body;

    QRegularExpressionMatch match = sourceRegex.match(response);

    if (match.hasMatch()) {
        QString source = match.captured(1);
        Video video(source);
        video.addHeader("Referer", "https://" + Functions::getHostFromUrl(serverUrl));
        return { video };
    } else {
        qDebug() <<"Log (KimCartoon): No source found.";
    }


    return {};
}

