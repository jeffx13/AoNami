// #include "wco.h"

// QList<ShowData> WCOFun::search(Client *client, const QString &query, int page, int type) {
//     QList<ShowData> shows;
//     if (page > 1)
//         return shows;

//     QMap<QString, QString> data = {
//         {"catara", query},
//         {"konuara", "series"}
//     };
//     auto doc = client->post("https://www.wcofun.net/search", data, m_headers).toSoup();
//     auto items = doc.select("//ul[@class='items']/li/div[@class='img']/a");
//     for (const auto &item : items) {
//         auto img = item.selectFirst("./img");
//         QString coverUrl = img.attr("src");
//         QString title = img.attr("alt");
//         QString link = item.attr("href");
//         shows.emplaceBack(title, link, coverUrl, this, "", ShowData::ANIME);
//     }
//     return shows;
// }

// QList<ShowData> WCOFun::latest(Client *client, int page, int typeIndex) {
//     QList<ShowData> shows;
//     if (page > 1)
//         return shows;
//     auto doc = client->get("https://www.wcofun.net/last-50-recent-release", m_headers).toSoup();
//     auto items = doc.select("//ul[@class='items']/li");
//     for (const auto &item : items) {
//         auto img = item.selectFirst("./div[@class='img']/a/img");
//         auto anchor = item.selectFirst("./div[@class='recent-release-episodes']/a");
//         QString coverUrl = QString("https:") + img.attr("src");
//         QString title = Functions::substringBefore(anchor.text(), " Episode");
//         auto href = Functions::substringBefore(anchor.attr("href"), "-season");
//         href = Functions::substringBefore(href, "-episode");
//         QString link = hostUrl() + "anime" + href;
//         shows.emplaceBack(title, link, coverUrl, this, "", ShowData::ANIME);
//     }
//     return shows;
// }

// int WCOFun::loadDetails(Client *client, ShowData &show, bool getEpisodeCountOnly, bool fetchPlaylist) const {


//     auto doc = client->get(show.link, m_headers).toSoup();
//     if (!doc) return false;
//     auto episodesNodes = doc.select("//div[@id='sidebar_right3']/div[@class='cat-eps']/a");
//     if (episodesNodes.isEmpty()) return false;
//     if (getEpisodeCountOnly) return episodesNodes.size();

//     auto infoDiv = doc.selectFirst("//div[@id='sidebar_cat']");
//     show.description = infoDiv.selectFirst("./p").text();
//     auto genres = infoDiv.select("./a[@class='genre-buton']");
//     for (const auto &genre : genres) {
//         show.genres.push_back(genre.text());
//     }

//     if (!fetchPlaylist) return true;
//     static QRegularExpression re(R"((?:Season (\d+))?[^\d]+Episode (\d+)[-\s–]*(.*))");
//     int maxSeason = -1;
//     for (const auto &episodesNode : episodesNodes) {
//         auto match = re.match(episodesNode.text());
//         if (match.hasMatch()) {
//             int season = match.captured(1).toInt();
//             if (maxSeason == -1) {
//                 maxSeason = season;
//             }
//             if (maxSeason > 0 && season == 0)
//                 season = 1;
//             int episode = match.captured(2).toInt();
//             auto link = episodesNode.attr("href");
//             auto title = match.captured(3);
//             if (title.contains("English"))
//                 title = "";
//             show.addEpisode(season, episode, link, title);
//         } else {
//             oLog() << name() << "Failed to match episode" << episodesNode.text();
//         }
//     }
//     show.getPlaylist()->reverse();

//     return true;
// }

// QList<VideoServer> WCOFun::loadServers(Client *client, const PlaylistItem *episode) const {
//     QList<VideoServer> servers { {"Default", episode->link}};
//     return servers;
// }

// PlayItem WCOFun::extractSource(Client *client, VideoServer &server) {
//     PlayItem playItem;

//     auto iframeSrc = client->get(server.link, m_headers).toSoup().selectFirst("//div[@class='pcat-jwplayer']//iframe").attr("src");
//     QProcess process;
//     QString pythonCode = R"(import cloudscraper,re;scraper=cloudscraper.create_scraper();headers={'referer': 'https://www.wcofun.net/'};response=scraper.get(r'%1', headers=headers);url='https://embed.watchanimesub.net/'+re.findall(r'"(/inc/embed/getvidlink\.php\?[^\"]+)\"', response.text)[0];headers={'referer':r'%1','x-requested-with': 'XMLHttpRequest'};response=scraper.get(url, headers=headers).json();print(f'{response["server"]}/getvid?evid={response["enc"]}'))";
//     pythonCode = pythonCode.arg(iframeSrc);
//     process.start("C:\\Users\\Jeffx\\AppData\\Local\\Microsoft\\WindowsApps\\python.exe", QStringList() << "-c" << pythonCode); // "-c -" means reading from stdin
//     if (!process.waitForStarted()) {
//         oLog() << name() << "Failed to start python.";
//         return playItem;
//     }
//     process.waitForFinished();
//     QString output = process.readAllStandardOutput().trimmed();
//     playItem.videos.emplaceBack(output);
//     return playItem;
// }
