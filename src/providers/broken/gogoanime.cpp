// #include "gogoanime.h"
// #include "extractors/gogocdn.h"


// QList<ShowData> Gogoanime::search(Client *client, const QString &query, int page, int type)
// {
//     QList<ShowData> animes;
//     QString url = hostUrl() + "search.html?keyword=" + query + "&page=" + QString::number (page);
//     auto nodes = client->get(url).toSoup()
//                      .select("//ul[@class='items']/li/div[@class='img']/a");

//     for (auto &node : nodes) {
//         QString title = node.attr("title");
//         QString coverUrl = node.selectFirst("./img").attr("src");
//         QString link = node.attr("href");
//         animes.emplaceBack(title, link, coverUrl, this, "", ShowData::ANIME);
//     }

//     return animes;
// }

// QList<ShowData> Gogoanime::popular(Client *client, int page, int type) {
//     QList<ShowData> animes;
//     QString url = "https://ajax.gogocdn.net/ajax/page-recent-release-ongoing.html?page=" + QString::number(page);
//     auto animeNodes = client->get(url).toSoup().select ("//div[@class='added_series_body popular']/ul/li");

//     for (const auto &node:animeNodes) {
//         auto anchor = node.selectFirst("a");
//         QString link = anchor.attr("href");
//         QString coverUrl = anchor.selectFirst(".//div[@class='thumbnail-popular']").attr("style");
//         coverUrl = coverUrl.split ("'").at (1);
//         QString title = anchor.attr("title");
//         auto latestText=node.selectFirst(".//p[last()]/a").text();
//         animes.emplaceBack (title, link, coverUrl, this, latestText, ShowData::ANIME);
//     }

//     return animes;
// }

// QList<ShowData> Gogoanime::latest(Client *client, int page, int type) {
//     QList<ShowData> animes;
//     QString url = "https://ajax.gogocdn.net/ajax/page-recent-release.html?page=" + QString::number(page) + "&type=1" ;
//     auto nodes = client->get(url).toSoup().select("//ul[@class='items']/li");

//     for (auto &node : nodes) {
//         QString coverUrl = node.selectFirst(".//img").attr("src");
//         static QRegularExpression re{R"(([\w-]*?)(?:-\d{10})?\.)"};
//         auto lastSlashIndex = coverUrl.lastIndexOf("/");
//         auto id = re.match (coverUrl.mid(lastSlashIndex + 1));
//         if (!id.hasMatch()) {
//             cLog() << name() << "Unable to extract Id from" << coverUrl.mid(lastSlashIndex+1);
//             continue;
//         }
//         QString title = QString(node.selectFirst(".//p[@class='name']/a").text()).trimmed().replace("\n", " ");
//         QString link = "/category/" + id.captured (1);
//         QString latestText = node.selectFirst (".//p[@class='episode']").text();
//         animes.emplaceBack (title, link, coverUrl, this, latestText, ShowData::ANIME);
//     }
//     return animes;
// }

// int Gogoanime::loadDetails(Client *client, ShowData &show, bool getEpisodeCountOnly, bool fetchPlaylist) const
// {
//     auto url = hostUrl() + show.link;
//     auto doc = client->get(url).toSoup();
//     if (!doc) return false;

//     if (loadInfo) {
//         if (auto pNodes = doc.select ("//div[@class='description']/p"); !pNodes.empty()) {
//             for (const auto &node : pNodes) {
//                 show.description += node.text().replace ("\n"," ").trimmed() + "\n\n";
//             }
//             show.description = show.description.trimmed();
//         } else {
//             auto descriptionNode = doc.selectFirst("//div[@class='description']");
//             show.description = descriptionNode.text().replace ("\n"," ").trimmed();
//         }
//         show.status = doc.selectFirst ("//span[contains(text(),'Status')]/following-sibling::a").text();
//         show.releaseDate = doc.selectFirst ("//span[contains(text() ,'Released')]/following-sibling::text()").text();
//         auto genreNodes = doc.select ("//span[contains(text(),'Genre')]/following-sibling::a");
//         for (const auto &genreNode : genreNodes) {
//             QString genre = genreNode.attr("title").replace ("\n"," ");
//             show.genres.push_back (genre);
//         }
//     }

//     if (!getPlaylist && !getEpisodeCount) return true;

//     int lastEpisode = doc.selectFirst ("//ul[@id='episode_page']/li[last()]/a").attr("ep_end").toInt();

//     if (getPlaylist) {
//         QString animeId = doc.selectFirst ("//input[@id='movie_id']").attr("value");
//         QString alias = doc.selectFirst ("//input[@id='alias_anime']").attr("value");
//         // std::string epStart = lastEpisode > 1000 ? std::to_string(lastEpisode - 99) : "0";
//         QString epStart = "0";
//         QString link = "https://ajax.gogocdn.net/ajax/load-list-episode?ep_start="
//                        + epStart + "&ep_end="  + QString::number(lastEpisode)
//                        + "&id=" + animeId + "&default_ep=0" + "&alias=" + alias;

//         auto episodeNodes = client->get(link).toSoup().select("//li/a");
//         if (episodeNodes.empty()) return false;
//         lastEpisode = episodeNodes.size();
//         for (int i=0; i<episodeNodes.size(); ++i) {
//             auto episodeNode = episodeNodes[episodeNodes.size() - i - 1];
//             QString title = episodeNode.selectFirst("./div[@class='name']/text()").text().trimmed();
//             float number = -1;
//             bool ok;
//             float intTitle = title.toFloat (&ok);
//             if (ok){
//                 number = intTitle;
//                 title = "";
//             }
//             QString link = episodeNode.attr("href").trimmed();
//             show.addEpisode(0, number, link, title);
//         }
//     }

//     return lastEpisode;
// }

// QString Gogoanime::getEpisodesLink(const CSoup &doc) const
// {
//     QString lastEpisode = doc.selectFirst ("//ul[@id='episode_page']/li[last()]/a").attr("ep_end");
//     QString animeId = doc.selectFirst ("//input[@id='movie_id']").attr("value");
//     return "https://ajax.gogocdn.net/ajax/load-list-episode?ep_start=0&ep_end=" + lastEpisode + "&id=" + animeId;
// }


// QList<VideoServer> Gogoanime::loadServers(Client *client, const PlaylistItem *episode) const
// {
//     QList<VideoServer> servers;
//     auto url = hostUrl() + episode->link;
//     auto serverNodes = client->get(url).toSoup()
//                            .select("//div[@class='anime_muti_link']/ul/li/a");

//     for (const auto &serverNode:serverNodes) {
//         QString link = serverNode.attr("data-video");
//         QString name = serverNode.selectFirst("./text()").text();
//         if (link.startsWith ("//")){
//             link = "https:" + link;
//         }
//         servers.emplaceBack (name, link);
//     }
//     return servers;
// }

// PlayItem Gogoanime::extractSource(Client *client, VideoServer &server) {
//     PlayItem playItem;

//     auto serverName = server.name.toLower();
//     if (serverName.contains ("vidstreaming") || serverName.contains ("gogo")) {
//         GogoCDN extractor;
//         auto source = extractor.extract(client, server.link);
//         playItem.sources.emplaceBack(source);
//     }
//     return playItem;
// }
