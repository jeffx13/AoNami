// #include "cineby.h"

// #include <BigInt.hpp>


// QList<ShowData> Cineby::search(Client *client, const QString &query, int page, int type) {
//     QString url = QString("https://db.cineby.app/3/search/multi?language=en&page=%1&query=%2&api_key=ad301b7cc82ffe19273e55e4d4206885")
//                       .arg(QString::number(page), QUrl::toPercentEncoding(query));
//     auto results = client->get(url, m_headers).toJsonObject()["results"].toArray();
//     QList<ShowData> shows;
//     for (const auto &item : results) {
//         auto showItem = item.toObject();
//         QString mediaType = showItem["media_type"].toString();
//         if (mediaType == "person") continue;
//         int type = mediaType == "tv" ? ShowData::TVSERIES : ShowData::MOVIE;
//         auto posterPath = showItem["poster_path"].toString();
//         QString coverUrl = posterPath.isEmpty() ? "" : "https://image.tmdb.org/t/p/w500/" + posterPath;
//         QString title = showItem["name"].toString();
//         if (title.isEmpty()) title = showItem["title"].toString();
//         QString link = mediaType + "/" + QString::number(showItem["id"].toInt());
//         shows.emplaceBack(title, link, coverUrl, this, "", type);
//     }
//     return shows;

// }

// QList<ShowData> Cineby::popular(Client *client, int page, int type) {
//     if (page != 1) return {};
//     // QString url = "https://db.cineby.app/3/discover/%1?page=%2&language=en&with_genres&with_original_language=en&api_key=ad301b7cc82ffe19273e55e4d4206885"
//     // url = url.

//     return getSection(client, 1);

// }

// QList<ShowData> Cineby::latest(Client *client, int page, int type) {
//     if (page != 1) return {};
//     return getSection(client, 0);
// }

// int Cineby::loadDetails(Client *client, ShowData &show, bool getEpisodeCountOnly, bool fetchPlaylist) const {
//     QString infoUrl = "https://db.cineby.app/3/" + show.link + "?append_to_response=credits,external_ids,similar,videos,recommendations,translations&language=en&api_key=ad301b7cc82ffe19273e55e4d4206885";

//     auto info = client->get(infoUrl, m_headers).toJsonObject();
//     show.releaseDate = info["first_air_date"].toString();
//     if (show.releaseDate.isEmpty()) show.releaseDate = info["release_date"].toString();

//     show.status = info["status"].toString();
//     show.description = info["overview"].toString();
//     auto mediaTypeAndId = show.link.split('/');
//     show.views = QString::number(info["popularity"].toDouble());
//     show.score = QString::number(info["vote_average"].toDouble());
//     auto genres = info["genres"].toArray();
//     for (const auto &genre: genres) {
//         show.genres.emplaceBack(genre.toObject()["name"].toString());
//     }
//     show.updateTime = info["last_air_date"].toString();
//     if (show.updateTime.isEmpty())
//         show.updateTime = "N/A";

//     if (!getPlaylist && !getEpisodeCount) return 0;



//     if (getPlaylist) {
//         auto year = show.releaseDate.split('-').first();
//         if (show.type == ShowData::MOVIE) {
//             auto url = QString("https://api.cineby.app/myflixerzupcloud/sources-with-title?title=%1&mediaType=%2&year=%3&episodeId=1&seasonId=1&tmdbId=%4")
//                            .arg(QUrl::toPercentEncoding(show.title), mediaTypeAndId.first(), year, mediaTypeAndId.last());
//             show.addEpisode(0, -1, url, "Movie");

//         } else if (show.type == ShowData::TVSERIES) {
//             int seasons = info["number_of_seasons"].toInt();
//             for (int i = 1; i <= seasons; i++) {
//                 QString seasonUrl = QString("https://db.cineby.app/3/%1/season/%2?language=en&api_key=ad301b7cc82ffe19273e55e4d4206885").arg(show.link, QString::number(i));
//                 auto season = client->get(seasonUrl, m_headers).toJsonObject();

//                 auto airDate = season["air_date"].toString();
//                 if (airDate.isEmpty()) continue;

//                 auto episodes = season["episodes"].toArray();
//                 for (const auto &episode : episodes) {
//                     auto episodeObj = episode.toObject();
//                     QString title = episodeObj["name"].toString();
//                     float number = episodeObj["episode_number"].toDouble();
//                     // QString id = QString::number(episodeObj["id"].toInt());
//                     QString link = QString("https://api.cineby.app/myflixerzupcloud/sources-with-title?title=%1&mediaType=%2&year=%3&episodeId=%4&seasonId=%5&tmdbId=%6")
//                                        .arg(QUrl::toPercentEncoding(show.title), mediaTypeAndId.first(), year, QString::number(episodeObj["episode_number"].toInt()), QString::number(i), mediaTypeAndId.last());

//                     show.addEpisode(seasons > 1 ? i : 0, number, link, title);
//                 }
//             }

//         }

//     }

//     return show.type == ShowData::TVSERIES ? info["number_of_episodes"].toInt() : 1;
// }

// QList<VideoServer> Cineby::loadServers(Client *client, const PlaylistItem *episode) const {

//     return {VideoServer("default", episode->link)};
// }

// PlayItem Cineby::extractSource(Client *client, VideoServer &server) {

//     PlayItem info;
//     auto response = client->get(server.link, m_headers);
//     if (response.body.startsWith('{')) {
//         auto errorJson = response.toJsonObject();
//         QString errorMessage = errorJson["message"].toString();
//         throw MyException(errorMessage, name());
//     }
//     WasmEngine& engine = getEngine(client);
//     auto decrypted = this->decrypt(engine, response.body.toStdString(), server.link.split("tmdbId=").last().toInt());

//     auto data = QJsonDocument::fromJson(QString::fromStdString(decrypted).toUtf8()).object();
//     auto sources = data["sources"].toArray();
//     for (const auto &source : sources) {
//         auto sourceObj = source.toObject();
//         auto link = sourceObj["url"].toString();
//         auto label = sourceObj["quality"].toString();
//         auto video = Video(link);
//         video.resolution = label;
//         video.addHeader("Referer", "https://www.cineby.app/");
//         video.addHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/131.0.0.0 Safari/537.36 Edg/131.0.0.0");
//         info.sources.push_back(video);
//     }

//     auto subtitles = data["subtitles"].toArray();
//     for (const auto &sub : subtitles) {
//         auto subObj = sub.toObject();
//         info.subtitles.emplaceBack(subObj["language"].toString(), QUrl::fromUserInput(subObj["url"].toString()));
//     }
//     return info;

// }

// WasmEngine &Cineby::getEngine(Client *client) {
//     static bool initialised = false;
//     static std::optional<WasmEngine> engine;
//     if (!initialised) {
//         QString wasmUrl = "https://www.cineby.app/release.wasm";
//         engine = WasmEngine::fromOnlineFile(client, wasmUrl);
//         // engine = WasmEngine::fromLocalFile(QUrl::fromLocalFile("C:/Users/Jeffx/Downloads/release.wasm"));
//         auto memory_export = engine.value().instance.value().get(engine.value().store, "memory");
//         if (!memory_export || !memory_export.has_value() || !std::holds_alternative<wasmtime::Memory>(memory_export.value())) {
//             throw MyException("Expected memory export 'memory'", "Cineby");
//         }
//         memory = std::get<wasmtime::Memory>(memory_export.value());
//         decrypt_func  = std::get<wasmtime::Func>(*engine.value().instance.value().get(engine.value().store, "decrypt"));
//         new_func = std::get<wasmtime::Func>(*engine.value().instance.value().get(engine.value().store, "__new"));
//         initialised = true;
//     }
//     return engine.value();
// }

// std::string Cineby::encode(BigInt input) const {
//     int n = (input % 100).to_int();
//     BigInt length = this->alphabet.size();
//     std::string e = this->alphabet;
//     std::string i{ alphabet[(n % length).to_int()] };
//     std::string r = i;
//     auto o = r + e;
//     e = shuffle(e, o);

//     std::string c;
//     BigInt temp = input;
//     BigInt base = e.length();

//     do {
//         c.insert(c.begin(), e[(temp % base).to_int()]);
//         temp /= base;
//     } while (temp > 0);

//     i+=c;
//     if (i.length() < 0) {
//         i.push_back(guards[0]);
//         r.push_back(guards[0]);
//     }

//     return i;
// }

// std::string Cineby::shuffle(std::string i, const std::string &e) const {
//     if (e.empty()) {
//         return i;
//     }
//     int n = 0, r = 0, a = 0;
//     for (int t = i.size() - 1; t > 0; t--, r++) {
//         r %= e.length();
//         n = static_cast<unsigned char>(e[r]);
//         a += n;
//         int s = (n + r + a) % t;
//         // Swap elements at indices t and s
//         std::swap(i[t], i[s]);

//     }

//     return i;
// }

// QList<ShowData> Cineby::getSection(Client *client, int section)
// {

//     auto homePage = client->get("https://www.cineby.app/").body;
//     auto buildId = homePage.split("buildId\":\"").last().split("\"").first();
//     QString url = "https://www.cineby.app/_next/data/%1/en.json" ;
//     url = url.arg(buildId);

//     auto response = client->get(url, m_headers);
//     auto results = response.toJsonObject()["pageProps"].toObject()["defaultSections"].toArray()[section].toObject()["movies"].toArray();
//     QList<ShowData> shows;
//     for (const auto &item : results) {
//         auto showItem = item.toObject();
//         QString title = showItem["title"].toString();
//         QString mediaType = showItem["mediaType"].toString();
//         int type = mediaType == "tv" ? ShowData::TVSERIES : ShowData::MOVIE;
//         auto poster = showItem["poster"].toString();
//         QString coverUrl = poster.isEmpty() ? "" : poster;
//         QString link = showItem["slug"].toString();
//         link = link.mid(1);
//         shows.emplaceBack(title, link, coverUrl, this, "", type);
//     }
//     return shows;
// }

// std::string Cineby::hash(const std::string &input) const {
//     // Convert a string to a vector of ASCII values
//     auto stringToAscii = [](const std::string& str) -> std::vector<int> {
//         std::vector<int> asciiValues;
//         for (char c : str) {
//             asciiValues.push_back(static_cast<int>(c));
//         }
//         return asciiValues;
//     };

//     // Hardcoded key string
//     std::string key = "5817deea68d131de99b8841851dea89b3462b1dfa5a4f98ee4f8";
//     std::vector<int> keyAscii = stringToAscii(key);

//     // XOR the input ASCII values with the key ASCII values
//     std::vector<int> hashedAscii;
//     for (char c : input) {
//         int charCode = static_cast<int>(c);
//         int xorValue = std::accumulate(keyAscii.begin(), keyAscii.end(), charCode, std::bit_xor<>());
//         hashedAscii.push_back(xorValue);
//     }

//     // Convert the XORed values to a hexadecimal string
//     std::ostringstream hexStream;
//     for (int val : hashedAscii) {
//         hexStream << std::setw(2) << std::setfill('0') << std::hex << (val & 0xFF);
//     }

//     return hexStream.str();
// }

// std::string Cineby::decrypt(WasmEngine& engine, const std::string &input, int tmdbId) const {
//     int length = input.length();

//     unsigned int r = new_func.value().call(const_cast<wasmtime::Store&>(engine.store), {wasmtime::Val(length << 1) , 2}).unwrap()[0].i32();

//     uint16_t* uint16View = reinterpret_cast<uint16_t*>(memory.value().data(const_cast<wasmtime::Store&>(engine.store)).data());
//     for (unsigned int a = 0; a < length; ++a){
//         int charCode = static_cast<unsigned char>(input[a]);
//         uint16View[(r >> 1) + a] = charCode;
//     }

//     auto results = decrypt_func.value().call(const_cast<wasmtime::Store&>(engine.store), {(int)r, (double)tmdbId}).unwrap();

//     uint32_t val = results[0].i32();

//     uint32_t* uint32View = reinterpret_cast<uint32_t*>(memory.value().data(const_cast<wasmtime::Store&>(engine.store)).data());
//     std::stringstream ss;
//     uint32_t end = (val + (uint32View[(val - 4) >> 2])) >> 1;
//     uint32_t start = val >> 1;

//     for (unsigned int i = start; i < end; ++i) {
//         ss << static_cast<char>(uint16View[i]);
//     }

//     auto hashed = hash(std::to_string(tmdbId) + "7a3d4f1ab3199649");

//     auto key = encode(hashed);
//     std::string decryptedData = Functions::decryptAES(ss.str(), key);
//     return decryptedData;
// }
