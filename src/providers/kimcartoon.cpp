
#include "kimcartoon.h"

#include <QTextDocument>

QVector<ShowData> Kimcartoon::search(Client *client, const QString &query, int page, int type) {
    if (page > 1)
        return {};
    QString url = baseUrl + "Search/Cartoon";
    auto doc = client->post(url, {}, {{"keyword", query}}).toSoup();
    auto showNodes =  doc.select("//div[@class='list-cartoon']/div/a[1]");
    return parseResults(showNodes);
}

QVector<ShowData> Kimcartoon::popular(Client *client, int page, int type) {
    QString url = baseUrl + "CartoonList/MostPopular" + "?page=" + QString::number(page);
    return filterSearch(client, url);
}

QVector<ShowData> Kimcartoon::latest(Client *client, int page, int type) {
    QString url = baseUrl + "CartoonList/LatestUpdate" + "?page=" + QString::number(page);
    return filterSearch(client, url);
}

QVector<ShowData> Kimcartoon::filterSearch(Client *client, const QString &url) {
    auto showNodes = client->get(url).toSoup().select("//div[@class='list-cartoon']/div/a[1]");
    if (showNodes.empty()) return {};
    return parseResults (showNodes);
}

int Kimcartoon::loadDetails(Client *client, ShowData &show, bool loadInfo, bool loadPlaylist, bool getEpisodeCount) const {
    auto doc = client->get(baseUrl + show.link).toSoup();
    if (loadInfo) {
        auto infoDiv = doc.selectFirst("//div[@class='barContent']");
        if (!infoDiv) return false;

        show.title = infoDiv.selectFirst(".//a[@class='bigChar']").text();
        show.title.replace ("\n", " ");

        auto descriptionParagraph= infoDiv.selectFirst(".//span[contains(text() ,'Summary')]/parent::p/following-sibling::p[1]");
        show.description = descriptionParagraph.text().replace('\n'," ").replace("&nbsp"," ").trimmed();

        auto dateAiredTextNode = doc.selectFirst ("//span[contains(text() ,'Date')]/following-sibling::text()");
        show.releaseDate = dateAiredTextNode.text();

        auto statusTextNode = infoDiv.selectFirst(".//span[contains(text() ,'Status')]/following-sibling::text()[1]");
        show.status = statusTextNode.text();

        auto viewsTextNode = infoDiv.selectFirst(".//span[contains(text() ,'Views')]/following-sibling::text()[1]");
        show.views = viewsTextNode.text();

        auto genreNodes = infoDiv.select("//span[contains(text(),'Genres')]/following-sibling::a");
        for (const auto &genreNode : genreNodes) {
            QString genre = genreNode.text();
            show.genres.push_back(genre.trimmed());
        }
    }


    if (!loadPlaylist) return true;
    QRegularExpression titleRegex(QString(show.title).replace (" ", "\\s*"));
    auto episodeNodes = doc.select("//table[@class='listing']//a");
    if (episodeNodes.empty ()) return false;
    if (getEpisodeCount) return episodeNodes.size();

    for (int i = episodeNodes.size() - 1; i >= 0; --i) {
        const auto *it = &episodeNodes[i];
        QStringList fullEpisodeName;
        auto episodeNameString = it->text().replace(titleRegex, "").replace("\n", "").trimmed();
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

        link = it->attr("href").replace(" ", "%20");
        show.addEpisode(0, number, link, title);
    }

    return true;
}

QVector<VideoServer> Kimcartoon::loadServers(Client *client, const PlaylistItem *episode) const {

    auto doc = client->get(baseUrl + episode->link).toSoup();
    auto serverNodes = doc.select("//select[@id='selectServer']/option");
    QList<VideoServer> servers;
    for (const auto &serverNode : serverNodes) {
        QString serverName = serverNode.text().trimmed();
        QString serverLink = serverNode.attr("value").replace(" ", "%20");;
        servers.emplaceBack (serverName, serverLink);
    }
    return servers;
}
PlayInfo Kimcartoon::extractSource(Client *client, const VideoServer &server) const {
    PlayInfo playInfo;

    auto script = client->get(baseUrl + server.link).toSoup()
                      .selectFirst("//div[@id='divContentVideo']/script");
    if (!script) return {};

    QString serverUrl = Functions::substring(script.text(), ".src = '",  "';");
    Functions::httpsIfy(serverUrl);
    auto response = client->get(serverUrl, {{"sec-fetch-dest", "iframe"}}).body;

    static QRegularExpression sourceRegex{"sources: \\[\\{file:\"(.+?)\"\\}\\]"};
    QRegularExpressionMatch match = sourceRegex.match(response);
    if (match.hasMatch()) {
        QString source = match.captured(1);
        playInfo.sources.emplaceBack(source);
        playInfo.sources.last().addHeader("Referer", "https://" + Functions::getHostFromUrl(serverUrl));
    } else {
        qDebug() <<"Log (KimCartoon): No source found.";
    }

    return playInfo;
}

QVector<ShowData> Kimcartoon::parseResults(const QVector<CSoup::Node> &showNodes) {
    QVector<ShowData> shows;
    for (const auto &node:showNodes) {
        auto anchor = node.selectFirst(".");
        QString title = anchor.selectFirst (".//span").text().replace ('\n'," ").trimmed();
        QString coverUrl = anchor.selectFirst("./img").attr("src");
        if (coverUrl.startsWith ('/')) coverUrl = baseUrl + coverUrl;
        QString link = anchor.attr("href");
        shows.emplaceBack(title, link, coverUrl, this, "", ShowData::ANIME);
    }
    return shows;
}

