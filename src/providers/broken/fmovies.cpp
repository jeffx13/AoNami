// #include "fmovies.h"
// #include "providers/extractors/vidsrcextractor.h"
// #include <network/csoup.h>
// #include "core/showdata.h"
// #include <QClipboard>
// #include <QGuiApplication>


// QList<ShowData> FMovies::search(Client *client, const QString &query, int page, int type) {
//     QString cleanedQuery = query;
//     cleanedQuery.replace(" ", "+");
//     return filterSearch(client, "keyword=" + cleanedQuery + "&sort=most_relevance", page, type);
// }

// QList<ShowData> FMovies::popular(Client *client, int page, int type) {
//     return filterSearch(client, "keyword=&sort=most_watched", page, type);
// }

// QList<ShowData> FMovies::latest(Client *client, int page, int type) {
//     return filterSearch(client, "keyword=&sort=recently_updated", page, type);
// }

// QList<ShowData> FMovies::filterSearch(Client *client, const QString &filter, int page, int type) {
//     QList<ShowData> shows;
//     auto url = hostUrl + "/filter?" + filter + "&type%5B%5D=" + (type == ShowData::TVSERIES ? "tv" : "movie") + "&page=" + QString::number(page);
//     auto document = client->get(url).toSoup();
//     auto nodes = document.select("//div[@class='i-movie']/div");
//     for (auto &node : nodes) {
//         auto anchor = node.selectFirst("./a");
//         QString title = node.selectFirst("./div/h5").text();
//         QString link = anchor.attr("href");
//         QString coverUrl = anchor.selectFirst("./div/img").attr("data-src");
//         shows.emplaceBack (title, link, coverUrl, this, "", type);
//     }



//     return shows;
// }

// int FMovies::loadDetails(Client *client, ShowData &show, bool getEpisodeCountOnly, bool fetchPlaylist) const
// {
//     auto url = hostUrl + show.link;
//     auto doc= client->get(url).toSoup();
//     if (!doc) return -1;
//     if (loadInfo) {
//         auto info = doc.selectFirst("//section[@id='w-info']/div[@class='info']");
//         auto detail = info.selectFirst("./div[@class='detail']");
//         auto descElement = info.selectFirst("./div[@class='description cts-wrapper']");
//         auto descElementDiv = descElement.selectFirst(".");
//         auto desc = descElementDiv ? descElementDiv.text() : descElement.text();
//         auto extraInfoDivs = detail.select("./div");
//         QString extraInfo = "";
//         for (const auto &div: extraInfoDivs) {
//             extraInfo += div.text() + '\n';
//         }
//         extraInfo = extraInfo.trimmed();
//         show.title = info.selectFirst("./h1[@class='name']").text();
//         // auto mediaDetail = utils.getDetail(mediaTitle);
//         show.score = info.selectFirst("./div[@class='rating-box']").text();
//         show.releaseDate = detail.selectFirst ("./div/span[@itemprop='dateCreated']").text();
//         show.description = QString("%1").arg(desc); //todo ? mediadetail?

//         auto genreNodes = detail.select("./div[div[contains(text(), 'Genre:')]]/span");
//         for (const auto &genreNode : genreNodes) {
//             QString genre = genreNode.text();
//             show.genres.push_back (genre);
//         }
//     }


//     if (!getPlaylist) return 0;

//     auto id = doc.selectFirst("//div[@data-id]").attr("data-id");
//     auto vrf = vrfEncrypt(id);

//     vrfHeaders["Referer"] =  hostUrl + show.link;

//     auto response = client->get(hostUrl + "/ajax/episode/list/"+id+"?vrf=" + vrf, vrfHeaders).toJsonObject();
//     auto document = CSoup::parse(response["result"].toString());
//     auto seasons = document.select("//ul[@class='range episodes']");
//     for (const auto &season : seasons) {
//         int seasonNumber = seasons.size() > 1 ? season.attr("data-season").toInt() : 0;
//         auto episodeNodes = season.select("./li");
//         for (const auto &ep : episodeNodes) {
//             auto a = ep.selectFirst("./a");
//             float number = -1;
//             bool ok;
//             float intTitle = a.attr("data-num").toFloat (&ok);
//             if (ok){
//                 number = intTitle;
//             }
//             auto title = a.selectFirst("./span").text().trimmed();
//             auto id = a.attr("data-id");
//             auto url = hostUrl + a.attr("href");
//             show.addEpisode(seasonNumber, number, QString("%1;%2").arg(id, url), title);
//         }

//     }

//     return true;
// }

// QList<VideoServer> FMovies::loadServers(Client *client, const PlaylistItem *episode) const
// {
//     QList<VideoServer> servers;
//     auto data = episode->link.split(';');
//     auto id = data.first();
//     auto vrf = vrfEncrypt(id);
//     vrfHeaders["Referer"] =  data.last();
//     auto response = Client(nullptr).get(hostUrl + "/ajax/server/list/" + id + "?vrf=" + vrf, vrfHeaders).toJsonObject();
//     auto document = CSoup::parse(response["result"].toString());

//     auto serverNodes = document.select("//span[@class='server']");
//     for (const auto &server : serverNodes) {
//         auto name = server.selectFirst(".//span").text().trimmed();
//         auto vrf = vrfEncrypt(server.attr("data-link-id"));
//         auto serverUrl = hostUrl + "/ajax/server/" + server.attr("data-link-id") + "?vrf=" + vrf;
//         auto result = Client(nullptr).get(serverUrl, vrfHeaders).toJsonObject()["result"].toObject();
//         auto encrypted = result["url"].toString();
//         auto decrypted = vrfDecrypt(encrypted);
//         servers.emplaceBack(name, id + ";" + decrypted);
//     }
//     return servers;
// }



// PlayItem FMovies::extractSource(Client *client, VideoServer &server) {
//     PlayItem playItem;
//     qDebug() << server.name;
//     if (server.name == "VidCloud" || server.name == "FMCloud") {
//         auto data = server.link.split(';');
//         auto url = data.last();
//         auto id = data.first();
//         auto subsJsonArray = Client(nullptr).get(hostUrl + "/ajax/episode/subtitles/" + id).toJsonArray();
//         for (const auto &object: subsJsonArray) {
//             auto sub = object.toObject();
//             auto label = sub["label"].toString();
//             auto file = sub["file"].toString();
//             if (label == "English") {
//                 playItem.subtitles.emplaceFront(file, label);
//             } else {
//                 playItem.subtitles.emplaceBack(file, label);
//             }
//         }
//         Vidsrcextractor extractor;
//         playItem.sources = extractor.videosFromUrl(url, server.name, "", playItem.subtitles);
//     }
//     return playItem;
// }

// QString FMovies::base64UrlSafeEncode(const QByteArray &input) const
// {
//     using namespace CryptoPP;

//     std::string encoded;
//     StringSource ss(reinterpret_cast<const byte*>(input.data()), input.size(), true,
//                     new CryptoPP::Base64Encoder(
//                         new StringSink(encoded),
//                         false // No padding
//                         )
//                     );

//     // Crypto++'s Base64Encoder will not add padding by default if false is passed to it.
//     // Remove padding manually if necessary (Crypto++ doesn't add it here because false was passed).

//     return QString::fromStdString(encoded).replace("+", "-").replace("/", "_");
// }

// QByteArray FMovies::base64UrlSafeDecode(const QString &input) const
// {
//     using namespace CryptoPP;

//     std::string encoded = input.toStdString();

//     // Make URL safe: replace - with +, _ with /
//     for (char& c : encoded) {
//         if (c == '-') c = '+';
//         else if (c == '_') c = '/';
//     }

//     // Decode Base64
//     std::string decoded;
//     StringSource ss(encoded, true,
//                     new Base64Decoder(
//                         new StringSink(decoded)
//                         )
//                     );

//     return QByteArray::fromStdString(decoded);
// }

// // QString FMovies::vrfEncrypt(const QString &input) const {
// //     using namespace CryptoPP;

// //     // RC4 Key
// //     byte rc4Key[] = "Ij4aiaQXgluXQRs6";
// //     SecByteBlock key(rc4Key, 16);

// //     // RC4 Encryption
// //     Weak::ARC4::Encryption rc4Encryption(key, key.size());
// //     std::string cipherText;
// //     StringSource ss1(input.toStdString(), true,
// //                      new StreamTransformationFilter(rc4Encryption,
// //                                                     new StringSink(cipherText)
// //                                                     )
// //                      );

// //     // Convert encrypted string to QByteArray
// //     QByteArray vrf = QByteArray::fromStdString(cipherText);

// //     // First Base64 encode
// //     QString base64Encoded = base64UrlSafeEncode(vrf);
// //     vrf = base64Encoded.toUtf8();

// //     // Second Base64 encode
// //     base64Encoded = base64UrlSafeEncode(vrf);
// //     vrf = base64Encoded.toUtf8();

// //     // Reverse the QByteArray
// //     std::reverse(vrf.begin(), vrf.end());

// //     // Third Base64 encode
// //     base64Encoded = base64UrlSafeEncode(vrf);
// //     vrf = base64Encoded.toUtf8();

// //     // Apply vrfShift
// //     vrf = vrfShift(vrf);

// //     // Convert to QString and URL encode
// //     QString stringVrf = QString::fromUtf8(vrf);
// //     QString urlEncodedVrf = QUrl::toPercentEncoding(stringVrf);

// //     return urlEncodedVrf;
// // }

// QString FMovies::vrfDecrypt(const QString &input) const
// {
//     using namespace CryptoPP;

//     // Base64 decode the input
//     QByteArray vrf = base64UrlSafeDecode(input);

//     // RC4 Key
//     byte rc4Key[] = "8z5Ag5wgagfsOuhz";
//     SecByteBlock key(rc4Key, 16);

//     // RC4 Decryption
//     Weak::ARC4::Decryption rc4Decryption(key, key.size());
//     std::string decryptedText;
//     StringSource ss1(reinterpret_cast<const byte*>(vrf.data()), vrf.size(), true,
//                      new StreamTransformationFilter(rc4Decryption,
//                                                     new StringSink(decryptedText)
//                                                     )
//                      );

//     // Convert decrypted text to QByteArray
//     vrf = QByteArray::fromStdString(decryptedText);

//     // URL decode the final string
//     QString stringVrf = QString::fromUtf8(vrf);
//     QString urlDecodedVrf = QUrl::fromPercentEncoding(stringVrf.toUtf8());

//     return urlDecodedVrf;
// }
