// #include "haitu.h"

// QList<ShowData> Haitu::search(Client *client, const QString &query, int page, int type)
// {
//     QString cleanedQuery = QUrl::toPercentEncoding (QString(query).replace (" ", "+"));
//     return filterSearch(client, cleanedQuery, "--", page);
// }

// QList<ShowData> Haitu::popular(Client *client, int page, int typeIndex)
// {
//     return filterSearch(client, QString::number(types[typeIndex]), "hits", page);
// }

// QList<ShowData> Haitu::latest(Client *client, int page, int typeIndex)
// {
//     return filterSearch(client, QString::number(types[typeIndex]), "time", page);
// }

// QList<ShowData> Haitu::filterSearch(Client *client, const QString &query, const QString &sortBy, int page) {
//     QString url = hostUrl() + (sortBy == "--" ? "vodsearch/": "vodshow/")
//                + query + "--" + sortBy + "------" + QString::number(page) + "---.html";

//     auto showNodes = client->get(url).toSoup()
//         .select("//div[@class='module-list']/div[@class='module-items']/div");

//     QList<ShowData> shows;
//     for (const auto &node : std::as_const(showNodes))
//     {
//         auto moduleItemCover = node.selectFirst(".//div[@class='module-item-cover']");

//         if (sortBy== "--" &&
//             moduleItemCover.selectFirst(".//span[@class='video-class']").text()== "伦理片")
//             continue;

//         auto img = moduleItemCover.selectFirst(".//div[@class='module-item-pic']/img");
//         QString title = img.attr("alt");
//         QString coverUrl = img.attr("data-src");
//         if(coverUrl.startsWith ('/')) {
//             coverUrl = hostUrl() + coverUrl;
//         }


//         QString link = moduleItemCover.selectFirst(".//div[@class='module-item-pic']//a").attr("href");
//         QString latestText;

//         if (sortBy == "--"){
//             latestText = node.selectFirst (".//a[@class='video-serial']").text();
//         } else {
//             latestText = node.selectFirst(".//div[@class='module-item-text']").text();
//         }

//         shows.emplaceBack(title, link, coverUrl, this, latestText);
//     }

//     return shows;
// }

// int Haitu::loadDetails(Client *client, ShowData &show, bool getEpisodeCountOnly, bool fetchPlaylist) const
// {
//     auto doc = client->get(hostUrl() + show.link).toSoup();
//     if (!doc) return false;

//     auto serverNodes = doc.select ("//div[@class='scroll-content']");
//     auto serverNameNodes = doc.select("//div[@class='module-heading']//div[@class='module-tab-content']/div");


//     if (getEpisodeCountOnly | fetchPlaylist) {
//         int res = parseMultiServers(show, serverNodes, serverNameNodes, getEpisodeCountOnly);
//         if (getEpisodeCountOnly) return res;
//     }

//     auto infoItems = doc.select ("//div[@class='video-info-items']/div");
//     if (infoItems.empty()) return false;
//     show.releaseDate = infoItems[2].text();
//     show.updateTime = infoItems[3].text();
//     show.updateTime = show.updateTime.split ("，").first();
//     show.status = infoItems[4].text();
//     show.score = infoItems[5].selectFirst(".//font").text();
//     show.description = infoItems[7].selectFirst(".//span").text().trimmed();
//     auto genreNodes = doc.select ("//div[@class='tag-link']/a");
//     for (const auto &genreNode : std::as_const(genreNodes)) {
//         show.genres += genreNode.text();
//     }

//     return true;
// }

// QList<VideoServer> Haitu::loadServers(Client *client, const PlaylistItem *episode) const
// {
//     auto serversString = episode->link.split (";");
//     QList<VideoServer> servers;
//     for (auto& serverString: serversString) {
//         auto serverNameAndLink = serverString.split (" ");
//         QString serverName = serverNameAndLink.first();
//         QString serverLink = serverNameAndLink.last();
//         servers.emplaceBack(serverName, serverLink);
//     }
//     return servers;
// }

// PlayItem Haitu::extractSource(Client *client, VideoServer &server)
// {
//     PlayItem playItem;
//     QString response = client->get(hostUrl() + server.link).body;
//     static QRegularExpression player_aaaa_regex{R"(player_aaaa=(\{.*?\})</script>)"};
//     QRegularExpressionMatch match = player_aaaa_regex.match(response);

//     if (match.hasMatch()) {
//         QString link = QJsonDocument::fromJson(match.captured (1).toUtf8()).object()["url"].toString();
//         auto m3u8 = client->get(link).body.split ('\n');
//         if (m3u8[0].trimmed().compare("#EXTM3U") != 0) {
//             qWarning() << "Haitu: broken server" << server.name;
//             return playItem;
//         }
//         playItem.videos.emplaceBack(link);
//     } else {
//         qWarning() << "Haitu failed to extract m3u8";
//     }




//     return playItem;

// }
