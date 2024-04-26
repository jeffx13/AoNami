#include "gogoanime.h"
#include "Extractors/gogocdn.h"


QList<ShowData> Gogoanime::search(const QString &query, int page, int type)
{
    QList<ShowData> animes;

    QString url = hostUrl + "search.html?keyword=" + query + "&page=" + QString::number (page);
    auto animeNodes = NetworkClient::get(url, {})
                          .document().select("//ul[@class='items']/li");
    if (animeNodes.empty())
        return animes;

    for (pugi::xpath_node_set::const_iterator it = animeNodes.begin(); it != animeNodes.end(); ++it) {
        auto anchor = it->node().select_node(".//p[@class='name']/a");
        QString title = anchor.node().attribute("title").as_string();
        QString coverUrl = it->node().select_node(".//img").node().attribute("src").as_string();
        QString link = anchor.node().attribute ("href").as_string();
        animes.emplaceBack(title, link, coverUrl, this);
    }

    return animes;
}

QList<ShowData> Gogoanime::popular(int page, int type) {
    QList<ShowData> animes;
    QString url = "https://ajax.gogocdn.net/ajax/page-recent-release-ongoing.html?page=" + QString::number(page);
    auto animeNodes = NetworkClient::get (url).document().select ("//div[@class='added_series_body popular']/ul/li");
    for (pugi::xpath_node_set::const_iterator it = animeNodes.begin(); it != animeNodes.end(); ++it) {
        pugi::xpath_node anchor = it->node().select_node ("a");
        QString link = anchor.node().attribute("href").as_string();
        QString coverUrl = QString(anchor.node().select_node(".//div[@class='thumbnail-popular']").node().attribute ("style").as_string());
        coverUrl = coverUrl.split ("'").at (1);
        QString title = anchor.node().attribute ("title").as_string();
        animes.emplaceBack (title, link, coverUrl, this);
        animes.last().latestTxt = it->node().select_node(".//p[last()]/a").node().child_value();
    }

    return animes;
}

QList<ShowData> Gogoanime::latest(int page, int type) {
    QList<ShowData> animes;
    QString url = "https://ajax.gogocdn.net/ajax/page-recent-release.html?page=" + QString::number(page) + "&type=1" ;
    auto response = NetworkClient::get(url);

    pugi::xpath_node_set animeNodes = response.document().select("//ul[@class='items']/li");
    if (animeNodes.empty()) return animes;

    for (pugi::xpath_node_set::const_iterator it = animeNodes.begin(); it != animeNodes.end(); ++it) {
        QString coverUrl = it->node().select_node(".//img").node().attribute("src").as_string();
        static QRegularExpression re{R"(([\w-]*?)(?:-\d{10})?\.)"};
        auto lastSlashIndex =  coverUrl.lastIndexOf("/");
        auto id = re.match (coverUrl.mid(lastSlashIndex + 1));
        if (!id.hasMatch()) {
            qDebug() << "Unable to extract Id from" << coverUrl.mid(lastSlashIndex+1);
            continue;
        }

        QString title = QString(it->node().select_node (".//p[@class='name']/a").node().child_value()).trimmed().replace("\n", " ");
        QString link = "/category/" + id.captured (1);
        QString latestText = it->node().select_node (".//p[@class='episode']").node().child_value();
        animes.emplaceBack (title, link, coverUrl, this, latestText);
    }


    return animes;
}

bool Gogoanime::loadDetails(ShowData &show, bool getPlaylist) const
{
    CSoup doc = NetworkClient::get(hostUrl + show.link).document();

    int lastEpisode = std::stoi(doc.selectFirst ("//ul[@id='episode_page']/li[last()]/a").node().attribute ("ep_end").as_string());
    QString animeId = doc.selectFirst ("//input[@id='movie_id']").node().attribute ("value").as_string();
    QString alias = doc.selectFirst ("//input[@id='alias_anime']").node().attribute ("value").as_string();
    // std::string epStart = lastEpisode > 1000 ? std::to_string(lastEpisode - 99) : "0";
    QString epStart = "0";
    QString link = "https://ajax.gogocdn.net/ajax/load-list-episode?ep_start="
                   + epStart + "&ep_end="  + QString::number(lastEpisode)
                   + "&id=" + animeId + "&default_ep=0" + "&alias=" + alias;

    if (auto pNodes = doc.select ("//div[@class='description']/p"); !pNodes.empty()) {
        for (pugi::xpath_node_set::const_iterator it = pNodes.begin(); it != pNodes.end(); ++it)
        {
            show.description += QString(it->node().child_value()).replace ("\n"," ").trimmed() + "\n\n";
        }
        show.description = show.description.trimmed();
    } else {
        auto descriptionNode = doc.selectFirst("//div[@class='description']");
        if (!descriptionNode.node().first_child() && std::string(descriptionNode.node().child_value()).empty()) {
            qDebug() << "The div is empty.\n";
        } else {
            show.description = QString(descriptionNode.node().child_value()).replace ("\n"," ").trimmed();
        }
    }
    show.status = doc.selectFirst ("//span[contains(text(),'Status')]/following-sibling::a").node().child_value();

    if (pugi::xml_node statusTextNode =
        doc.selectFirst ("//span[contains(text(),'Status')]/following-sibling::a").node();
        statusTextNode.type() == pugi::node_pcdata)
        show.releaseDate = statusTextNode.child_value();

    if (pugi::xml_node releasedTextNode =
        doc.selectFirst ("//span[contains(text() ,'Released')]/following-sibling::text()").node();
        releasedTextNode.type() == pugi::node_pcdata)
        show.releaseDate = releasedTextNode.value();

    pugi::xpath_node_set genreNodes = doc.select ("//span[contains(text(),'Genre')]/following-sibling::a");
    for (pugi::xpath_node_set::const_iterator it = genreNodes.begin(); it != genreNodes.end(); ++it)
    {
        QString genre = QString(it->node().attribute ("title").as_string()).replace ("\n"," ");
        show.genres.push_back (genre);
    }

    if (!getPlaylist) return true;
    pugi::xpath_node_set episodeNodes = NetworkClient::get(link).document().select("//li/a");
    if (episodeNodes.empty()) return false;

    for (pugi::xpath_node_set::const_iterator it = episodeNodes.end() - 1; it != episodeNodes.begin() - 1; --it) {
        QString title = QString::fromStdString (it->node().select_node (".//div").node().child_value()).replace("EP", "").trimmed();
        float number = -1;
        bool ok;
        float intTitle = title.toFloat (&ok);
        if (ok){
            number = intTitle;
            title = "";
        }
        QString link = it->node().attribute ("href").value();
        show.addEpisode(number, link, title);
    }


    return true;
}

QString Gogoanime::getEpisodesLink(const CSoup &doc) const
{
    QString lastEpisode = doc.selectFirst ("//ul[@id='episode_page']/li[last()]/a").node().attribute ("ep_end").as_string();
    QString animeId = doc.selectFirst ("//input[@id='movie_id']").node().attribute ("value").as_string();
    return "https://ajax.gogocdn.net/ajax/load-list-episode?ep_start=0&ep_end=" + lastEpisode + "&id=" + animeId;
}

int Gogoanime::getTotalEpisodes(const QString& link) const {
    CSoup doc = NetworkClient::get(hostUrl + link).document();
    return NetworkClient::get(getEpisodesLink(doc)).document().select("//ul/li/a").size();
}

QList<VideoServer> Gogoanime::loadServers(const PlaylistItem *episode) const
{
    QList<VideoServer> servers;
    pugi::xpath_node_set serverNodes = NetworkClient::get(hostUrl + episode->link).document().select("//div[@class='anime_muti_link']/ul/li/a");
    for (pugi::xpath_node_set::const_iterator it = serverNodes.begin(); it != serverNodes.end(); ++it)
    {
        QString link = it->node().attribute("data-video").as_string();
        QString name = QString(it->node().child_value()).trimmed();
        if (link.startsWith ("//")){
            link = "https:" + link;
        }
        servers.emplaceBack (name, link);
    }
    return servers;
}

QList<Video> Gogoanime::extractSource(const VideoServer &server) const {
    auto serverName = server.name.toLower();
    if (serverName.contains ("vidstreaming") ||
        serverName.contains ("gogo")) {
        GogoCDN extractor;
        auto source = extractor.extract(server.link);
        return { Video(source) };
    }
    return {};
}
