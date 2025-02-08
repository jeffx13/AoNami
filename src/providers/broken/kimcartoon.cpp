
#include "kimcartoon.h"

#include <QTextDocument>

QVector<ShowData> Kimcartoon::search(Client *client, const QString &query, int page, int type) {
    if (page > 1) return {};
    QString url = hostUrl() + "Search/?s=" + QUrl::toPercentEncoding(query);
    auto doc = client->post(url, {{"s", query}}).toSoup();
    return parseResults(doc);
}

QVector<ShowData> Kimcartoon::popular(Client *client, int page, int type) {
    QString url = hostUrl() + "CartoonList/MostPopular" + "?page=" + QString::number(page);
    return parseResults (client->get(url).toSoup());
}

QVector<ShowData> Kimcartoon::latest(Client *client, int page, int type) {
    QString url = hostUrl() + "CartoonList/LatestUpdate" + "?page=" + QString::number(page);
    return parseResults (client->get(url).toSoup());
}


int Kimcartoon::loadDetails(Client *client, ShowData &show, bool loadInfo, bool getPlaylist, bool getEpisodeCount) const {
    auto doc = client->get(show.link).toSoup();
    if (loadInfo) {
        auto infoDiv = doc.selectFirst("//div[@class='barContent full']");
        if (!infoDiv) return false;

        show.title = infoDiv.selectFirst(".//a[@class='bigChar']").text();
        show.title.replace ("\n", " ");

        auto descriptionParagraph= infoDiv.selectFirst(".//div[@class='summary'][1]/p");
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
            genre = genre.replace("Cartoon", "").trimmed();
            show.genres.push_back(genre);
        }
    }


    if (!getPlaylist && !getEpisodeCount) return true;
    QRegularExpression titleRegex(QString(show.title).replace (" ", "\\s*"));
    auto episodeNodes = doc.select("//div[@class='full item_ep']//a");
    if (episodeNodes.empty ()) return false;
    int episodeCount = episodeNodes.size();

    if (getPlaylist) {
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
    }

    return episodeCount;
}

QVector<VideoServer> Kimcartoon::loadServers(Client *client, const PlaylistItem *episode) const {

    auto doc = client->get(episode->link).toSoup();
    int lastEqualPos = episode->link.lastIndexOf('=');
    QString id = episode->link.mid(lastEqualPos + 1);
    auto serverNodes = doc.select("//select[@id='selectServer']/option");

    QList<VideoServer> servers;
    for (const auto &serverNode : serverNodes) {
        QString serverName = serverNode.text().trimmed();
        QString serverLink = serverNode.attr("sv") + ";" + id;
        servers.emplaceBack (serverName, serverLink);
    }
    return servers;
}
PlayInfo Kimcartoon::extractSource(Client *client, VideoServer &server) {
    PlayInfo playInfo;
    auto serverData = server.link.split(";");
    auto url = hostUrl() + "ajax/anime/load_episodes_v2?s=" + serverData.first();
    auto value = client->post(url, {{"episode_id", serverData.last()}}, {{"referer", hostUrl()}})
                     .toJsonObject()["value"].toString();
    auto doc = CSoup::parse(value);
    if (!doc) {
        return playInfo;
    }
    auto iframe = doc.selectFirst("//iframe").attr("src");
    if (iframe.startsWith ("//")) iframe = "https" + iframe;
    auto response = client->get(iframe, {{"referer", hostUrl()}}).body;
    static QRegularExpression urlPattern(R"("file":"([^"]+)\")");
    auto src = urlPattern.match(response).captured(1);
    playInfo.sources.emplaceBack(src);
    return playInfo;
}

QVector<ShowData> Kimcartoon::parseResults(const CSoup &doc) {
    auto showNodes =  doc.select("//div[@class='list-cartoon']/div/a[1]");
    QVector<ShowData> shows;
    for (const auto &node:showNodes) {
        QString title = node.selectFirst ("./h2").text().replace ('\n'," ").trimmed();
        QString coverUrl = node.selectFirst("./img").attr("src");
        if (coverUrl.startsWith ('/')) coverUrl = hostUrl() + coverUrl;
        QString link = node.attr("href");
        shows.emplaceBack(title, link, coverUrl, this, "", ShowData::ANIME);
    }
    return shows;
}

