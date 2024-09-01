// #include "nivod.h"

// #include <QCryptographicHash>

// QList<ShowData> Nivod::filterSearch(int page, const QString &sortBy, int type,
//                                     const QString &regionId,
//                                     const QString &langId,
//                                     const QString &yearRange) {

//     QString channel = type == ShowData::DOCUMENTARY ? "6" : QString::number(type); //FIX?

//     QMap<QString, QString> data = {{"sort_by", sortBy},
//                                    {"channel_id", channel},
//                                    {"show_type_id", "0"},
//                                    {"region_id", "0"},
//                                    {"lang_id", "0"},
//                                    {"year_range", " "},
//                                    {"start", QString::number((page-1) * 20)}};

//     QJsonArray responseJson = invokeAPI("https://api.nivodz.com/show/filter/WEB/3.2", data)["list"].toArray();
//     return parseShows(responseJson);
// }

// QList<ShowData> Nivod::search(Client *client, const QString &query, int page, int type) {

//     QMap<QString, QString> data = {{"keyword", query},
//                                    {"start", QString::number(--page * 20)},
//                                    {"cat_id", "1"},
//                                    {"keyword_type", "0"}};

//     QJsonArray responseJson = invokeAPI("https://api.nivodz.com/show/search/WEB/3.2", data)["list"].toArray();
//     return parseShows(responseJson);
// }

// QList<ShowData> Nivod::parseShows(const QJsonArray &showArrayJson) {
//     QList<ShowData> results;
//     foreach (const QJsonValue &value, showArrayJson) {
//         QJsonObject item = value.toObject();
//         int tvType;
//         QString channelName = item["channelName"].toString();
//         if (channelName == "电影") {
//             tvType = ShowData::MOVIE;
//         } else if (channelName == "电视剧") {
//             tvType = ShowData::TVSERIES;
//         } else if (channelName == "综艺") {
//             tvType = ShowData::VARIETY;
//         } else if (channelName == "动漫") {
//             tvType = ShowData::ANIME;
//         } else if (channelName == "纪录片") {
//             tvType = ShowData::DOCUMENTARY;
//         } else {
//             qDebug() << "Log (Nivod): Cannot infer show type from" << channelName;
//             tvType = ShowData::NONE;
//         }

//         QString title = item["showTitle"].toString();
//         QString coverUrl = item["showImg"].toString();
//         QString link = item["showIdCode"].toString();
//         QString latestTxt;
//         if (!item["episodesTxt"].isUndefined() && !item["episodesTxt"].isNull()) {
//             latestTxt = item["episodesTxt"].toString();
//         }
//         // QJsonDocument doc(item);
//         // QString jsonString = doc.toJson(QJsonDocument::Indented);
//         // qDebug().noquote() << jsonString;

//         results.emplaceBack(title, link, coverUrl, this, latestTxt, tvType);
//     }

//     return results;
// }

// QJsonObject Nivod::getInfoJson(const QString &link) const {
//     return invokeAPI("https://api.nivodz.com/show/detail/WEB/3.3",
//                    {{"show_id_code", link}, {"episode_id", "0"}})["entity"].toObject();
// }

// bool Nivod::loadDetails(Client *client, ShowData &show, bool loadInfo, bool loadPlaylist, bool getEpisodeCount) const {
//     QJsonObject infoJson = getInfoJson(show.link);
//     if (infoJson.isEmpty()) return false;

//     show.description = infoJson["showDesc"].toString();
//     show.genres += infoJson["showTypeName"].toString();
//     show.status = infoJson["episodesTxt"].toString();
//     //        auto actors =
//     //        QString::fromStdString(infoJson["actors"].get<std::string>());
//     show.releaseDate = QString::number(infoJson["postYear"].toInt(69));
//     show.views = QString::number(infoJson["hot"].toInt(69));
//     show.score = QString::number(infoJson["rating"].toInt(69));
//     show.updateTime = infoJson["episodesUpdateDesc"].toString();

//     if (!getPlaylist) return true;

//     foreach (const QJsonValue &item, infoJson["plays"].toArray()) {
//         auto episode = item.toObject();
//         QString title = episode["displayName"].toString();
//         float number = -1;
//         bool ok;
//         float intTitle = title.toFloat(&ok);
//         if (ok) {
//             number = intTitle;
//             title = "";
//         }
//         QString link = show.link + "&" + episode["playIdCode"].toString();
//         show.addEpisode(0, number, link, title);
//     }

//     return true;
// }

// PlayInfo Nivod::extractSource(Client *client, const VideoServer &server) const {
//     auto codes = server.link.split('&');
//     QMap<QString, QString> data = {{"play_id_code", codes.last()},
//                                    {"show_id_code", codes.first()},
//                                    {"oid", "1"},
//                                    {"episode_id", "0"}};
//     auto responseJson = invokeAPI("https://api.nivodz.com/show/play/info/WEB/3.3", data);
//     auto source = responseJson["entity"]
//                       .toObject()["plays"]
//                       .toArray()[0]
//                       .toObject()["playUrl"]
//                       .toString();
//     // QJsonDocument doc(responseJson);
//     // QString jsonString = doc.toJson(QJsonDocument::Indented);
//     // qDebug().noquote() << jsonString;

//     // qDebug() << QString::fromStdString(playUrl);

//     PlayInfo playInfo;
//     playInfo.sources.emplaceBack(source);
//     return playInfo;
// }

// QJsonObject Nivod::invokeAPI(const QString &url, const QMap<QString, QString> &data) const {

//     QString signQuery = _QUERY_PREFIX;
//     QString secretKey = "2x_Give_it_a_shot";
//     QMapIterator<QString, QString> queryIterator(queryMap);
//     while (queryIterator.hasNext()) {
//         queryIterator.next();
//         signQuery += queryIterator.key() + "=" + queryIterator.value() + "&";
//     }
//     signQuery += _BODY_PREFIX;
//     QMapIterator<QString, QString> bodyIterator(data);
//     while (bodyIterator.hasNext()) {
//         bodyIterator.next();
//         signQuery += bodyIterator.key() + "=" + bodyIterator.value() + "&";
//     }

//     QString input = signQuery + _SECRET_PREFIX + secretKey;
//     QString sign = MD5(input);

//     QString postUrl = url + "?_ts=" + _mts +
//                    "&app_version=1.0&platform=3&market_id=web_nivod&"
//                    "device_code=web&versioncode=1&oid=" +
//                    _oid + "&sign=" + sign;

//     auto response = Client::post(postUrl, m_headers, data);
//     if (response.code != 200)
//         return QJsonObject{};
//     auto decryptedResponse = decryptedByDES(response.body.toStdString());
//     QJsonParseError error;
//     QJsonDocument jsonData = QJsonDocument::fromJson(decryptedResponse.c_str(), &error);
//     if (error.error != QJsonParseError::NoError) {
//         qWarning() << "JSON parsing error:" << error.errorString();
//         return QJsonObject{};
//     }
//     return jsonData.object();
// }


// std::string Nivod::decryptedByDES(const std::string &input) const {
//      std::string key = "diao.com";
//      std::vector<byte> keyBytes(key.begin(), key.end());
//      std::vector<byte> inputBytes;
//      for (size_t i = 0; i < input.length(); i += 2) {
//          byte byte =
//              static_cast<unsigned char>(std::stoi(input.substr(i, 2),
//              nullptr, 16));
//          inputBytes.push_back(byte);
//      }

//     size_t length = inputBytes.size();
//     size_t padding = length % 8 == 0 ? 0 : 8 - length % 8;
//     inputBytes.insert(inputBytes.end(), padding, 0);

//     std::vector<byte> outputBytes(length + padding, 0);
//     CryptoPP::ECB_Mode<CryptoPP::DES>::Decryption decryption(keyBytes.data(),
//                                                              keyBytes.size());
//     CryptoPP::ArraySink sink(outputBytes.data(), outputBytes.size());
//     CryptoPP::ArraySource source(inputBytes.data(), inputBytes.size(), true,
//                                  new CryptoPP::StreamTransformationFilter(
//                                      decryption, new
//                                      CryptoPP::Redirector(sink)));
//     std::string decrypted(outputBytes.begin(), outputBytes.end());
//     size_t pos = decrypted.find_last_of('}');
//     if (pos != std::string::npos) {
//         decrypted = decrypted.substr(0, pos + 1);
//     }
//     return decrypted;
// }

// QString Nivod::MD5(const QString &str) const {
//     // Convert the input string to UTF-8 encoding and calculate its MD5 hash
//     QByteArray byteArray = str.toUtf8();
//     QByteArray hash = QCryptographicHash::hash(byteArray, QCryptographicHash::Md5);

//     // Convert the binary hash to a hexadecimal string
//     QString output = hash.toHex();

//     return output;
// }
