#include "wco.h"

QList<VideoServer> WCOFun::loadServers(Client *client, const PlaylistItem *episode) const {
    QList<VideoServer> servers { {"default", episode->link}};
    return servers;
}

PlayInfo WCOFun::extractSource(Client *client, VideoServer &server) const {
    PlayInfo playInfo;
    // auto UA = "Mozilla/5.0 (Linux; Android 8.0.0; moto g(6) play Build/OPP27.91-87) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/69.0.3497.100 Mobile Safari/537.36";
    QMap<QString, QString> headers{{"referer", hostUrl()}};
    auto iframeSrc = client->get(server.link, headers).toSoup().selectFirst("//div[@class='pcat-jwplayer']//iframe").attr("src");
    QProcess process;
    QString pythonCode = R"(import cloudscraper,re;scraper=cloudscraper.create_scraper();headers={'referer': 'https://www.wcofun.net/'};response=scraper.get(r'%1', headers=headers);url='https://embed.watchanimesub.net/'+re.findall(r'"(/inc/embed/getvidlink\.php\?[^\"]+)\"', response.text)[0];headers={'referer':r'%1','x-requested-with': 'XMLHttpRequest'};response=scraper.get(url, headers=headers).json();print(f'{response["server"]}/getvid?evid={response["enc"]}'))";
    pythonCode = pythonCode.arg(iframeSrc);
    process.start("C:\\Users\\Jeffx\\AppData\\Local\\Microsoft\\WindowsApps\\python.exe", QStringList() << "-c" << pythonCode); // "-c -" means reading from stdin
    if (!process.waitForStarted()) {
        qDebug() << "Failed to start process.";
        return playInfo;
    }
    process.waitForFinished();
    QString output = process.readAllStandardOutput().trimmed();
    playInfo.sources.emplaceBack(output);
    return playInfo;
}

QList<ShowData> WCOFun::search(Client *client, const QString &query, int page, int type) {
    QList<ShowData> shows;
    if (page > 1)
        return shows;

    auto url = "https://www.wcofun.net/search";
    QMap<QString, QString> data = {
        {"catara", query},
        {"konuara", "series"}
    };
    auto doc = client->post(url, data).toSoup();
    auto items = doc.select("//ul[@class='items']/li/div[@class='img']/a");
    for (const auto &item : items) {
        auto img = item.selectFirst("./img");
        QString coverUrl = img.attr("src");
        QString title = img.attr("alt");
        QString link = item.attr("href");
        shows.emplaceBack(title, link, coverUrl, this, "", ShowData::ANIME);
    }
    return shows;
}

QList<ShowData> WCOFun::latest(Client *client, int page, int type) {
    QList<ShowData> shows;
    if (page > 1)
        return shows;
    auto url = "https://www.wcofun.net/last-50-recent-release";
    auto doc = client->get(url).toSoup();
    auto items = doc.select("//ul[@class='items']/li");
    for (const auto &item : items) {
        auto img = item.selectFirst("./div[@class='img']/a/img");
        auto anchor = item.selectFirst("./div[@class='recent-release-episodes']/a");
        QString coverUrl = QString("https:") + img.attr("src");
        QString title = Functions::substringBefore(anchor.text(), " Episode");
        auto href = Functions::substringBefore(anchor.attr("href"), "-season");
        href = Functions::substringBefore(href, "-episode");
        QString link = hostUrl() + "anime" + href;
        shows.emplaceBack(title, link, coverUrl, this, "", ShowData::ANIME);
    }
    return shows;
}

int WCOFun::loadDetails(Client *client, ShowData &show, bool loadInfo, bool getPlaylist, bool getEpisodeCount) const {

    auto doc = client->get(show.link).toSoup();
    if (!doc) return false;
    if (loadInfo) {
        auto infoDiv = doc.selectFirst("//div[@id='sidebar_cat']");
        show.description = infoDiv.selectFirst("./p").text();
        auto genres = infoDiv.select("./a[@class='genre-buton']");
        for (const auto &genre : genres) {
            show.genres.push_back(genre.text());
        }
    }

    if (!getPlaylist && !getEpisodeCount) return true;
    static QRegularExpression re(R"((?:Season (\d+))?[^\d]+Episode (\d+)[-\s–]*(.*))");
    auto episodesNodes = doc.select("//div[@id='sidebar_right3']/div[@class='cat-eps']/a");
    if (episodesNodes.isEmpty()) return false;
    int episodeCount = episodesNodes.size();

    if (getPlaylist) {
        int maxSeason = -1;
        for (const auto &episodesNode : episodesNodes) {
            auto match = re.match(episodesNode.text());
            if (match.hasMatch()) {
                int season = match.captured(1).toInt();
                if (maxSeason == -1) {
                    maxSeason = season;
                }
                if (maxSeason > 0 && season == 0)
                    season = 1;
                int episode = match.captured(2).toInt();
                auto link = episodesNode.attr("href");
                auto title = match.captured(3);
                if (title.contains("English"))
                    title = "";
                show.addEpisode(season, episode, link, title);
            } else {
                qDebug() << "Failed to match episode" << episodesNode.text();
            }
        }
        show.getPlaylist()->reverse();
    }
    return episodeCount;
}
