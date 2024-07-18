#include "gogoanime.h"
#include "extractors/gogocdn.h"


QList<ShowData> Gogoanime::search(const QString &query, int page, int type)
{
    QList<ShowData> animes;
    QString url = baseUrl + "search.html?keyword=" + query + "&page=" + QString::number (page);
    auto document = CSoup::connect(url);
    auto nodes = document.select("//ul[@class='items']/li/div[@class='img']/a");
    for (auto &node : nodes) {
        QString title = node.attr("title");
        QString coverUrl = node.selectFirst("./img").attr("src");
        QString link = node.attr("href");
        animes.emplaceBack(title, link, coverUrl, this, "", ShowData::ANIME);
    }

    return animes;
}

QList<ShowData> Gogoanime::popular(int page, int type) {
    QList<ShowData> animes;
    QString url = "https://ajax.gogocdn.net/ajax/page-recent-release-ongoing.html?page=" + QString::number(page);
    auto document = CSoup::connect(url);
    auto animeNodes = document.select ("//div[@class='added_series_body popular']/ul/li");
    for (const auto &node:animeNodes) {
        auto anchor = node.selectFirst("a");
        QString link = anchor.attr("href");
        QString coverUrl = anchor.selectFirst(".//div[@class='thumbnail-popular']").attr("style");
        coverUrl = coverUrl.split ("'").at (1);
        QString title = anchor.attr("title");
        auto latestText=node.selectFirst(".//p[last()]/a").text();
        animes.emplaceBack (title, link, coverUrl, this, latestText, ShowData::ANIME);
    }

    return animes;
}

QList<ShowData> Gogoanime::latest(int page, int type) {
    QList<ShowData> animes;
    QString url = "https://ajax.gogocdn.net/ajax/page-recent-release.html?page=" + QString::number(page) + "&type=1" ;
    auto document = CSoup::connect(url);
    auto nodes = document.select("//ul[@class='items']/li");

    for (auto &node : nodes) {
        QString coverUrl = node.selectFirst(".//img").attr("src");
        static QRegularExpression re{R"(([\w-]*?)(?:-\d{10})?\.)"};
        auto lastSlashIndex = coverUrl.lastIndexOf("/");
        auto id = re.match (coverUrl.mid(lastSlashIndex + 1));
        if (!id.hasMatch()) {
            qDebug() << "Unable to extract Id from" << coverUrl.mid(lastSlashIndex+1);
            continue;
        }
        QString title = QString(node.selectFirst(".//p[@class='name']/a").text()).trimmed().replace("\n", " ");
        QString link = "/category/" + id.captured (1);
        QString latestText = node.selectFirst (".//p[@class='episode']").text();
        animes.emplaceBack (title, link, coverUrl, this, latestText, ShowData::ANIME);
    }
    return animes;
}

bool Gogoanime::loadDetails(ShowData &show, bool getPlaylist) const
{
    auto url = baseUrl + show.link;
    auto doc= CSoup::connect(url);
    if (!doc) return false;

    int lastEpisode = doc.selectFirst ("//ul[@id='episode_page']/li[last()]/a").attr("ep_end").toInt();
    QString animeId = doc.selectFirst ("//input[@id='movie_id']").attr("value");
    QString alias = doc.selectFirst ("//input[@id='alias_anime']").attr("value");
    // std::string epStart = lastEpisode > 1000 ? std::to_string(lastEpisode - 99) : "0";
    QString epStart = "0";
    QString link = "https://ajax.gogocdn.net/ajax/load-list-episode?ep_start="
                   + epStart + "&ep_end="  + QString::number(lastEpisode)
                   + "&id=" + animeId + "&default_ep=0" + "&alias=" + alias;

    if (auto pNodes = doc.select ("//div[@class='description']/p"); !pNodes.empty()) {
        for (const auto &node : pNodes) {
            show.description += node.text().replace ("\n"," ").trimmed() + "\n\n";
        }
        show.description = show.description.trimmed();
    } else {
        auto descriptionNode = doc.selectFirst("//div[@class='description']");
        show.description = descriptionNode.text().replace ("\n"," ").trimmed();
    }
    show.status = doc.selectFirst ("//span[contains(text(),'Status')]/following-sibling::a").text();
    show.releaseDate =doc.selectFirst ("//span[contains(text() ,'Released')]/following-sibling::text()").text();
    auto genreNodes = doc.select ("//span[contains(text(),'Genre')]/following-sibling::a");
    for (const auto &genreNode : genreNodes)
    {
        QString genre = genreNode.attr("title").replace ("\n"," ");
        show.genres.push_back (genre);
    }


    if (!getPlaylist) return true;

    auto episodeDoc = CSoup::connect(link);
    auto episodeNodes = episodeDoc.select("//li/a");
    if (episodeNodes.empty()) return false;

    for (const auto &episodeNode:episodeNodes) {
        QString title = episodeNode.selectFirst(".//div").text().replace("EP", "").trimmed();
        float number = -1;
        bool ok;
        float intTitle = title.toFloat (&ok);
        if (ok){
            number = intTitle;
            title = "";
        }
        QString link = episodeNode.attr("href").trimmed();
        show.addEpisode(0, number, link, title);
    }


    return true;
}

QString Gogoanime::getEpisodesLink(const CSoup &doc) const
{
    QString lastEpisode = doc.selectFirst ("//ul[@id='episode_page']/li[last()]/a").attr("ep_end");
    QString animeId = doc.selectFirst ("//input[@id='movie_id']").attr("value");
    return "https://ajax.gogocdn.net/ajax/load-list-episode?ep_start=0&ep_end=" + lastEpisode + "&id=" + animeId;
}

int Gogoanime::getTotalEpisodes(const QString& link) const {
    CSoup doc = CSoup::connect(link);
    if (!doc) return 0;
    doc = CSoup::connect(getEpisodesLink(doc));
    return doc.select("//ul/li/a").size();
}

QList<VideoServer> Gogoanime::loadServers(const PlaylistItem *episode) const
{
    QList<VideoServer> servers;
    auto url = baseUrl + episode->link;
    auto document = CSoup::connect(url);
    auto serverNodes = document.select("//div[@class='anime_muti_link']/ul/li/a");
    for (const auto &serverNode:serverNodes) {
        QString link = serverNode.attr("data-video");
        QString name = serverNode.text().trimmed();
        if (link.startsWith ("//")){
            link = "https:" + link;
        }
        servers.emplaceBack (name, link);
    }
    return servers;
}

PlayInfo Gogoanime::extractSource(const VideoServer &server) const {
    PlayInfo playInfo;

    auto serverName = server.name.toLower();
    if (serverName.contains ("vidstreaming") || serverName.contains ("gogo")) {
        GogoCDN extractor;
        auto source = extractor.extract(server.link);
        playInfo.sources.emplaceBack(source);
    }
    return playInfo;
}
