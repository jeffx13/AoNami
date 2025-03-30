#include "tangrenjie.h"





QList<ShowData> Tangrenjie::search(Client *client, const QString &query, int page, int type) {
    QList<ShowData> shows;
    QString url = QString("https://www.chinatownfilm.com/vodsearch/%1----------%2---.html").arg(QUrl::toPercentEncoding(query), QString::number(page));
    auto soup = client->get(url).toSoup();
    if (!soup) return shows;
    auto showNodes = soup.select("//div[@class='hl-item-div']//a[@class='hl-item-thumb hl-lazy']");
    for (auto &node:showNodes) {
        auto title = node.attr("title");
        auto coverUrl = node.attr("data-original");
        auto link = node.attr("href");
        shows.emplaceBack(title, link, coverUrl, this);
    }
    return shows;

}

QList<ShowData> Tangrenjie::popular(Client *client, int page, int type) {
    QList<ShowData> shows;

    QString url = QString("https://www.chinatownfilm.com/vodshow/%1--hits------%2---.html").arg(QString::number(typeMap[type]), QString::number(page));

    auto soup = client->get(url, m_headers).toSoup();
    if (!soup) return shows;
    auto showNodes = soup.select("//ul[@class='hl-vod-list clearfix']/li/a");
    for (auto &node:showNodes) {
        auto title = node.attr("title");
        auto coverUrl = node.attr("data-original");
        if (coverUrl.startsWith("/"))
            coverUrl = hostUrl() + coverUrl;
        auto link = node.attr("href");
        shows.emplaceBack(title, link, coverUrl, this);
    }
    return shows;
}

QList<ShowData> Tangrenjie::latest(Client *client, int page, int type) {
    QList<ShowData> shows;

    QString url = QString("https://www.chinatownfilm.com/vodshow/%1--time------%2---.html").arg(QString::number(typeMap[type]), QString::number(page));
    auto soup = client->get(url, m_headers).toSoup();
    if (!soup) return shows;
    auto showNodes = soup.select("//ul[@class='hl-vod-list clearfix']/li/a");
    for (auto &node:showNodes) {
        auto title = node.attr("title");
        auto coverUrl = node.attr("data-original");
        if (coverUrl.startsWith("/"))
            coverUrl = hostUrl() + coverUrl;
        auto link = node.attr("href");
        shows.emplaceBack(title, link, coverUrl, this);
    }
    return shows;

}

int Tangrenjie::loadDetails(Client *client, ShowData &show, bool getEpisodeCountOnly, bool fetchPlaylist) const {
    auto soup = client->get(hostUrl() + show.link, m_headers).toSoup();
    if (!soup) return 0;

    auto serverNameNodes = soup.select("//div[@class='hl-plays-from hl-tabs swiper-wrapper clearfix']/a");
    auto serverNodes = soup.select("//div[@class='hl-list-wrap']");
    if (getEpisodeCountOnly | fetchPlaylist) {
        int res = parseMultiServers(show, serverNodes, serverNameNodes, getEpisodeCountOnly);
        if (getEpisodeCountOnly) return res;
    }

    auto infoNodes = soup.select("//ul[@class='clearfix']/li");
    if (!infoNodes.isEmpty()) {
        show.releaseDate = infoNodes[4].selectFirst("./text()").text();
        show.coverUrl = soup.selectFirst("//span[@class='hl-item-thumb hl-lazy']").attr("data-original");
        show.updateTime = infoNodes[10].selectFirst("./text()").text();
        show.description = infoNodes[11].selectFirst("./text()").text();
    }

    return true;
}

QList<VideoServer> Tangrenjie::loadServers(Client *client, const PlaylistItem *episode) const {
    auto serversString = episode->link.split(";");
    QList<VideoServer> servers;
    for (auto& serverString: serversString) {
        auto serverNameAndLink = serverString.split(" ");
        QString serverName = serverNameAndLink.first();
        QString serverLink = serverNameAndLink.last();
        servers.emplaceBack (serverName, serverLink);
    }
    return servers;
}

PlayInfo Tangrenjie::extractSource(Client *client, VideoServer &server) {
    PlayInfo playInfo;
    QString iframeSrc = client->get(hostUrl() + server.link, m_headers).toSoup().selectFirst("//iframe").attr("src");
    auto response = client->get(hostUrl() + iframeSrc, m_headers).body;
    static QRegularExpression player_aaaa_regex{R"(player_aaaa=(\{.*?\})</script>)"};
    QRegularExpressionMatch match = player_aaaa_regex.match(response);

    if (match.hasMatch()) {
        QString link = QJsonDocument::fromJson(match.captured (1).toUtf8()).object()["url"].toString();
        auto m3u8 = client->get(link, m_headers).body.split('\n');
        if (m3u8[0].trimmed().compare("#EXTM3U") != 0) {
            oLog() << name() << "broken server" << server.name;
            return playInfo;
        }
        QUrl source = link;
        playInfo.sources.emplaceBack(source);
    } else {
        oLog() << name() << "failed to extract m3u8";
    }

    return playInfo;
}
