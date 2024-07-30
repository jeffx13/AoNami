
#include "kimcartoon.h"

#include <QTextDocument>

QVector<ShowData> Kimcartoon::search(const QString &query, int page, int type) {
    if (page > 1)
        return {};
    QString url = baseUrl + "Search/Cartoon";
    auto response = Client::post(url, {}, {{"keyword", query}});
    auto showNodes =  CSoup::parse(response.body)
                         .select("//div[@class='list-cartoon']/div/a[1]");

    return parseResults (showNodes);
}

QVector<ShowData> Kimcartoon::popular(int page, int type) {
    QString url = baseUrl + "CartoonList/MostPopular" + "?page=" + QString::number(page);
    return filterSearch(url);
}

QVector<ShowData> Kimcartoon::latest(int page, int type) {
    QString url = baseUrl + "CartoonList/LatestUpdate" + "?page=" + QString::number(page);
    return filterSearch(url);
}

QVector<ShowData> Kimcartoon::filterSearch(const QString &url) {
    auto showNodes = CSoup::connect(url)
                         .select("//div[@class='list-cartoon']/div/a[1]");
    if (showNodes.empty()) return {};
    return parseResults (showNodes);
}

bool Kimcartoon::loadDetails(ShowData &show, bool getPlaylist) const {
    auto doc = CSoup::connect(baseUrl + show.link);
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

    QRegularExpression titleRegex(QString(show.title).replace (" ", "\\s*"));

    if (!getPlaylist) return true;

    auto episodeNodes = doc.select("//table[@class='listing']//a");
    if (episodeNodes.empty ()) return false;

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

        link = it->attr("href").replace(" ", "%20");;
        // link.replace(" ", "%20");
        show.addEpisode(0, number, link, title);
    }

    return true;
}

QVector<VideoServer> Kimcartoon::loadServers(const PlaylistItem *episode) const {

    auto doc = CSoup::connect(baseUrl + episode->link);
    auto serverNodes = doc.select("//select[@id='selectServer']/option");
    QList<VideoServer> servers;
    for (const auto &serverNode : serverNodes) {
        QString serverName = serverNode.text().trimmed();
        QString serverLink = serverNode.attr("value").replace(" ", "%20");;
        servers.emplaceBack (serverName, serverLink);
    }
    return servers;
}
PlayInfo Kimcartoon::extractSource(const VideoServer &server) const {
    PlayInfo playInfo;

    auto script = CSoup::connect(baseUrl + server.link)
                      .selectFirst("//div[@id='divContentVideo']/script");
    if (!script) return {};

    QString serverUrl = Functions::substring(script.text(), ".src = '",  "';");
    Functions::httpsIfy(serverUrl);
    auto response = Client::get(serverUrl, {{"sec-fetch-dest", "iframe"}}).body;

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

