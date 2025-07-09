#include "seedbox.h"



QList<ShowData> SeedBox::popular(Client *client, int page, int typeIndex) {
    if (page > 1) return {};
    auto response = client->get(baseUrl + "/storage/downloads/rtorrent/", headers).toSoup();
    auto items = response.select("//ul[@id='items']/li[@class='item folder']/a");
    QList<ShowData> shows;
    for (const auto &item : std::as_const(items)) {
        auto link = item.attr("href");
        QString title = item.selectFirst("./span[@class='label']").text();
        auto coverResponse = client->get(baseUrl + link + "cover.txt", headers);
        QString cover = "";
        if (coverResponse.code == 200) {
            cover = coverResponse.body;
        }
        shows.emplaceBack(title, link, cover, this);
    }
    return shows;
}

QList<ShowData> SeedBox::latest(Client *client, int page, int typeIndex) {
    return popular(client, page, typeIndex);
}

int SeedBox::loadDetails(Client *client, ShowData &show, bool getEpisodeCountOnly, bool fetchPlaylist) const {
    if (getEpisodeCountOnly) {
        return 0;
    }

    auto currentUrl = baseUrl + show.link;

    auto items = client->get(currentUrl, headers).toSoup().select("//pre/a");
    items.removeFirst();

    for (int i = 0; i < items.size(); ++i) {
        auto link = items[i].attr("href");
        QString title = QUrl::fromPercentEncoding(link.toUtf8());
        if (link.endsWith("/")) {
            auto subItems = client->get(currentUrl + "/" + link, headers).toSoup().select("//pre/a");
            subItems.removeFirst();
            for (int j = 0; j < subItems.size(); ++j) {
                QString subTitle = subItems[j].attr("href");
                if (!subTitle.endsWith("/")) {
                    auto episodeTitle = QUrl::fromPercentEncoding(link.toUtf8()) + " " + QUrl::fromPercentEncoding(subTitle.toUtf8());
                    show.addEpisode(i + 1, j + 1, currentUrl + "/" + link + subTitle, episodeTitle);
                }
            }
        } else {
            show.addEpisode(0, i + 1, currentUrl + "/" + link, title);
        }
    }
    return 1;
}

PlayItem SeedBox::extractSource(Client *client, VideoServer &server) {
    PlayItem playItem;
    playItem.headers = headers;
    playItem.videos.emplaceBack(Video(server.link));
    return playItem;
}
