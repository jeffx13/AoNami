#include "fmovies.h"
#include "extractors/vidsrcextractor.h"
#include <network/csoup.h>

#include <QClipboard>
#include <QGuiApplication>


inline QList<ShowData> FMovies::search(const QString &query, int page, int type) {
    QString cleanedQuery = query;
    cleanedQuery.replace(" ", "+");
    return filterSearch("keyword=" + cleanedQuery + "&sort=most_relevance", page, type);
}

inline QList<ShowData> FMovies::popular(int page, int type) {
    return filterSearch("keyword=&sort=most_watched", page, type);
}

inline QList<ShowData> FMovies::latest(int page, int type) {
    return filterSearch("keyword=&sort=recently_updated", page, type);
}

QList<ShowData> FMovies::filterSearch(const QString &filter, int page, int type) {
    QList<ShowData> shows;
    auto url = baseUrl + "/filter?" + filter + "&type%5B%5D=" + (type == ShowData::TVSERIES ? "tv" : "movie") + "&page=" + QString::number(page);
    auto document = CSoup::connect(url);
    auto nodes = document.select("//div[@class='movies items ']/div[@class='item']");

    for (auto &node : nodes) {
        auto anchor = node.selectFirst("./div[@class='meta']/a");
        QString title = anchor.text();
        QString link = anchor.attr("href");
        QString coverUrl = node.selectFirst("./div[@class='poster']//img").attr("data-src");
        shows.emplaceBack (title, link, coverUrl, this, "", type);
    }
    return shows;
}

bool FMovies::loadDetails(ShowData &show, bool getPlaylist) const
{
    auto url = baseUrl + show.link;
    auto doc= CSoup::connect(url);
    if (!doc) return false;

    auto info = doc.selectFirst("//section[@id='w-info']/div[@class='info']");
    auto detail = info.selectFirst("./div[@class='detail']");
    auto descElement = info.selectFirst("./div[@class='description cts-wrapper']");
    auto descElementDiv = descElement.selectFirst(".");
    auto desc = descElementDiv ? descElementDiv.text() : descElement.text();
    auto extraInfoDivs = detail.select("./div");
    QString extraInfo = "";
    for (const auto &div: extraInfoDivs) {
        extraInfo += div.text() + '\n';
    }
    extraInfo = extraInfo.trimmed();
    show.title = info.selectFirst("./h1[@class='name']").text();
    // auto mediaDetail = utils.getDetail(mediaTitle);
    show.score = info.selectFirst("./div[@class='rating-box']").text();
    show.releaseDate = detail.selectFirst ("./div/span[@itemprop='dateCreated']").text();
    show.description = QString("%1").arg(desc); //todo ? mediadetail?

    auto genreNodes = detail.select("./div[div[contains(text(), 'Genre:')]]/span");
    for (const auto &genreNode : genreNodes) {
        QString genre = genreNode.text();
        show.genres.push_back (genre);
    }

    if (!getPlaylist) return true;
    auto id = doc.selectFirst("//div[@data-id]").attr("data-id");
    auto vrf = vrfEncrypt(id);
    vrfHeaders["Referer"] =  baseUrl + show.link;

    auto response = Client::get(baseUrl + "/ajax/episode/list/"+id+"?vrf=" + vrf, vrfHeaders).toJson();
    auto document = CSoup::parse(response["result"].toString());
    auto seasons = document.select("//div[@class='body']/ul[@class='episodes']");
    for (const auto &season : seasons) {
        int seasonNumber = seasons.size() > 1 ? season.attr("data-season").toInt() : 0;
        auto episodeNodes = season.select("./li");
        for (const auto &ep : episodeNodes) {
            auto a = ep.selectFirst("./a");
            float number = -1;
            bool ok;
            float intTitle = a.attr("data-num").toFloat (&ok);
            if (ok){
                number = intTitle;
            }
            auto title = a.selectFirst("./span[2]").text().trimmed();
            auto id = a.attr("data-id");
            auto url = baseUrl + a.attr("href");
            show.addEpisode(seasonNumber, number, QString("%1;%2").arg(id, url), title);
        }

    }

    return true;
}

QList<VideoServer> FMovies::loadServers(const PlaylistItem *episode) const
{
    QList<VideoServer> servers;
    auto data = episode->link.split(';');
    auto id = data.first();
    auto vrf = vrfEncrypt(id);
    vrfHeaders["Referer"] =  data.last();
    auto response = Client::get(baseUrl + "/ajax/server/list/" + id + "?vrf=" + vrf, vrfHeaders).toJson();
    auto document = CSoup::parse(response["result"].toString());

    auto serverNodes = document.select("//ul[@class='servers']/li[@class='server']");
    for (const auto &server : serverNodes) {
        auto name = server.text().trimmed();
        auto vrf = vrfEncrypt(server.attr("data-link-id"));
        auto serverUrl = baseUrl + "/ajax/server/" + server.attr("data-link-id") + "?vrf=" + vrf;
        auto result = Client::get(serverUrl, vrfHeaders).toJson()["result"].toObject();
        auto encrypted = result["url"].toString();
        auto decrypted = vrfDecrypt(encrypted);
        servers.emplaceBack(name, id + ";" + decrypted);
    }
    return servers;
}



PlayInfo FMovies::extractSource(const VideoServer &server) const {
    PlayInfo playInfo;

    if (server.name == "Vidplay" || server.name == "MyCloud") {
        auto data = server.link.split(';');
        auto url = data.last();
        auto id = data.first();
        auto subsJsonArray = Client::get(baseUrl + "/ajax/episode/subtitles/" + id).toJsonArray();
        for (const auto &object: subsJsonArray) {
            auto sub = object.toObject();
            auto label = sub["label"].toString();
            auto file = sub["file"].toString();
            if (label == "English") {
                playInfo.subtitles.emplaceFront(file, label);
            } else {
                playInfo.subtitles.emplaceBack(file, label);
            }
        }
        Vidsrcextractor extractor;
        playInfo.sources = extractor.videosFromUrl(url, server.name, "", playInfo.subtitles);
    }
    return playInfo;
}
