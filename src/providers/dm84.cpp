#include "dm84.h"


QList<ShowData> Dm84::search(Client *client, const QString &query, int page, int type)
{
    QString cleanedQuery = QUrl::toPercentEncoding (QString(query).replace (" ", "+"));
    return filterSearch(client, cleanedQuery, "--", page);
}

QList<ShowData> Dm84::popular(Client *client, int page, int type)
{
    return filterSearch(client, 0, "hits", page);
}

QList<ShowData> Dm84::latest(Client *client, int page, int type)
{
    return filterSearch(client, 0, "time", page);
}

QList<ShowData> Dm84::filterSearch(Client *client, const QString &query, const QString &sortBy, int page) {
    // QString url = hostUrl() + (sortBy == "--" ? "vodsearch/": "vodshow/")
               // + query + "--" + sortBy + "------" + QString::number(page) + "---.html";
    // 1 国漫
    // 2 日漫
    // 3 欧美动漫
    // 4 电影
    QString url = QString("%1show-%2--%3----%4.html").arg(hostUrl(), "1", sortBy, QString::number(page));
    auto showNodes = client->get(url).toSoup().select("//div[@class='item']");

    QList<ShowData> shows;
    for (const auto &node : showNodes)
    {
        QString title = node.selectFirst("./a[@class='title']").text();
        auto imageAnchor = node.selectFirst("./a");
        QString coverUrl = imageAnchor.attr("data-bg");
        QString link = imageAnchor.attr("href");
        QString latestText;
        shows.emplaceBack(title, link, coverUrl, this, "");
    }

    return shows;
}

int Dm84::loadDetails(Client *client, ShowData &show, bool getEpisodeCountOnly, bool fetchPlaylist) const
{
    auto doc = client->get(hostUrl() + show.link).toSoup();
    if (!doc) return false;

    QVector<CSoup::Node> serverNodes =doc.select("//ul[contains(@class, 'play_list')]");
    QVector<CSoup::Node> serverNameNodes = doc.select("//ul[@class='tab_control play_from']/li");

    if (getEpisodeCountOnly | fetchPlaylist) {
        int res = parseMultiServers(show, serverNodes, serverNameNodes, getEpisodeCountOnly);
        if (getEpisodeCountOnly) return res;
    }

    // auto infoItems = doc.select ("//div[@class='video-info-items']/div");
    // if (infoItems.empty()) return false;
    show.releaseDate = doc.selectFirst("//p[@class='v_desc']/*[2]").text();
    // show.updateTime = infoItems[3].text();
    // show.updateTime = show.updateTime.split ("，").first();
    // show.status = infoItems[4].text();
    // show.score = infoItems[5].selectFirst(".//font").text();
    show.description = doc.selectFirst("//div[@id='intro']/p[4]").text();
    // auto genreNodes = doc.select ("//div[@class='tag-link']/a");
    // for (const auto &genreNode : genreNodes) {
    //     show.genres += genreNode.text();
    // }

    return true;
}

QList<VideoServer> Dm84::loadServers(Client *client, const PlaylistItem *episode) const
{
    auto serversString = episode->link.split (";");
    QList<VideoServer> servers;
    for (auto& serverString: serversString) {
        auto serverNameAndLink = serverString.split (" ");
        QString serverName = serverNameAndLink.first();
        QString serverLink = serverNameAndLink.last();
        servers.emplaceBack(serverName, serverLink);
    }
    return servers;
}

PlayItem Dm84::extractSource(Client *client, VideoServer &server)
{
    PlayItem playItem;
    QString iframeSrc = client->get(hostUrl() + server.link).toSoup().selectFirst("//div[@class='p_box']/iframe").attr("src");
    auto response = client->get(iframeSrc).body;
    static QRegularExpression regex(R"(url = "([^"]+).;\s+var t = "(\d+).;\s+var key = hhh\("([^"]+)\")");
    QRegularExpressionMatch match = regex.match(response);

    if (match.hasMatch()) {
        QMap<QString, QString> dataMap;

        // Populate the QMap with the data
        dataMap["url"] = match.captured(1);
        dataMap["t"] = match.captured(2);
        dataMap["key"] = hhh(match.captured(3));
        dataMap["act"] = "0";
        dataMap["play"] = "1";
        rLog() << "key" << match.captured(3);
        rLog() << "true Key" << dataMap["key"];

        QMap<QString, QString> headers;
        headers.insert("origin", "https://hhjx.hhplayer.com");
        headers.insert("referer", "https://hhjx.hhplayer.com/index.php");
        headers.insert("user-agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/134.0.0.0 Safari/537.36");
        headers.insert("x-requested-with", "XMLHttpRequest");

        auto source = client->post("https://hhjx.hhplayer.com/api.php", dataMap, headers).toJsonObject()["url"].toString();
        if (!source.isEmpty())
            playItem.videos.emplaceBack(source);
    } else {
        qWarning() << "Dm84 failed to extract m3u8";
    }






    return playItem;

}
