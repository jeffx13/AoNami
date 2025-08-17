#include "seedbox.h"
#include "app/config.h"

SeedBox::SeedBox(QObject *parent) : ShowProvider(parent) {
    auto config = Config::get();
    if(config.contains("seedbox_url")) {
        baseUrl = config["seedbox_url"].toString();
    }
    if (config.contains("seedbox_auth")){
        headers["Authorization"] = config["seedbox_auth"].toString();
    }
}

QList<ShowData> SeedBox::popular(Client *client, int page, int typeIndex) {
    if (page > 1) return {};
    auto response = client->get(baseUrl + "/storage/downloads/rtorrent/", headers).toSoup();
    auto items = response.select("//div[@id='fallback']/table/tr/td[@class='fb-n']/a").sliced(1);

    QList<ShowData> shows;
    for (const auto &item : std::as_const(items)) {
        auto link = item.attr("href");
        QString title = item.text();
        QString cover = "";
        try {
            auto coverResponse = client->get(baseUrl + link + "cover.txt", headers);
            if (coverResponse.code == 200) {
                cover = coverResponse.body;
            }
        } catch(...){}

        shows.emplaceBack(title, link, cover, this);
    }
    return shows;
}

QList<ShowData> SeedBox::latest(Client *client, int page, int typeIndex) {
    return popular(client, page, typeIndex);
}

int SeedBox::loadShow(Client *client, ShowData &show, bool getEpisodeCountOnly, bool getPlaylist, bool getInfo) const {
    auto items = client->get(baseUrl + show.link, headers).toSoup()
    .select("//div[@id='fallback']/table/tr/td[@class='fb-n']/a").sliced(1);
    if (getEpisodeCountOnly) {
        return items.count();
    }

    for (int i = 0; i < items.size(); ++i) {
        auto link = items[i].attr("href");
        if (link.endsWith(".txt")) continue;
        QString title = items[i].text();

        if (link.endsWith("/")) {
            auto subItems = client->get(baseUrl + link, headers).toSoup()
            .select("//div[@id='fallback']/table/tr/td[@class='fb-n']/a").sliced(1);

            for (int j = 0; j < subItems.size(); ++j) {
                QString subItemLink = subItems[j].attr("href");
                QString subItemTitle = subItems[j].text();
                if (!subItemLink.endsWith("/")) {
                    show.addEpisode(i + 1, j + 1, baseUrl + subItemLink, subItemTitle);
                }
            }
        } else {
            show.addEpisode(0, i + 1, baseUrl + link, title);
        }
    }
    return items.size() - 1;
}

PlayInfo SeedBox::extractSource(Client *client, VideoServer &server) {
    PlayInfo playItem;
    playItem.headers = headers;
    playItem.videos.emplaceBack(server.link);
    return playItem;
}
