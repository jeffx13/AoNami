#include "anikoto.h"
#include <QUrl>
#include <QRegularExpression>
#include <QJsonArray>
#include "core/network/hlsproxy.h"
#include "registry.h"

REGISTER_PROVIDER(Anikoto, 0)

// Anikoto (anikototv.to) is an aniwave-style site. Every token the client needs (an episode's
// `data-ids`, a server's `data-link-id`) is served in the HTML, so there's no client-side crypto.
// The chain: episode list -> server/list (from data-ids) -> server?get (embed url) -> embed page's
// getSources[New] -> plaintext m3u8. The CDN 403s without a Referer, so we forward the embed origin.

QList<ShowData> Anikoto::parseShowList(const QString &html) {
    QList<ShowData> shows;
    auto doc = CSoup::parse(html);
    if (!doc) return shows;
    auto posters = doc.select("//div[contains(@class,'poster') and @data-tip]");
    for (const auto &p : std::as_const(posters)) {
        QString id = p.attr("data-tip");
        if (id.isEmpty()) continue;
        auto img = p.selectFirst(".//img");
        QString cover = img ? img.attr("src") : QString();
        QString title = img ? img.attr("alt") : QString();
        auto name = p.selectFirst("..//a[contains(@class,'d-title')]");
        if (name) {
            QString t = name.text().trimmed();
            if (!t.isEmpty()) title = t;
        }
        if (title.isEmpty()) continue;
        shows.emplaceBack(title, id, cover, this, "", ShowData::ANIME);
    }
    return shows;
}

QList<ShowData> Anikoto::search(Client *client, const QString &query, int page, int type) {
    Q_UNUSED(type);
    if (query.trimmed().isEmpty()) return {};
    QString url = hostUrl() + "filter?keyword=" + QUrl::toPercentEncoding(query)
                + "&page=" + QString::number(page);
    return parseShowList(client->get(url, m_headers).body);
}

QList<ShowData> Anikoto::popular(Client *client, int page, int typeIndex) {
    Q_UNUSED(typeIndex);
    QString url = hostUrl() + "ajax/home/widget/trending?page=" + QString::number(page);
    return parseShowList(client->get(url, m_headers).toJsonObject().value("result").toString());
}

QList<ShowData> Anikoto::latest(Client *client, int page, int typeIndex) {
    Q_UNUSED(typeIndex);
    QString url = hostUrl() + "ajax/home/widget/updated-sub?page=" + QString::number(page);
    return parseShowList(client->get(url, m_headers).toJsonObject().value("result").toString());
}

int Anikoto::loadShow(Client *client, ShowData &show, bool getEpisodeCountOnly, bool getPlaylist, bool getInfo) const {
    QString epUrl = hostUrl() + "ajax/episode/list/" + show.link + "?vrf=";
    QString epHtml = client->get(epUrl, m_headers).toJsonObject().value("result").toString();
    auto doc = CSoup::parse(epHtml);
    auto eps = doc ? doc.select("//a[@data-ids and @data-num]") : QVector<CSoup::Node>{};

    if (getEpisodeCountOnly && !getPlaylist && !getInfo) return eps.size();

    if (getPlaylist) {
        for (const auto &ep : std::as_const(eps)) {
            QString ids = ep.attr("data-ids");
            if (ids.isEmpty()) continue;
            float num = ep.attr("data-num").toFloat();
            // The stable `data-ids` blob is the episode's server key - stored as the episode link.
            show.addEpisode(0, num, ids, ep.attr("data-title").trimmed());
        }
    }

    if (getInfo) {
        auto tip = CSoup::parse(client->get(hostUrl() + "ajax/anime/tooltip/" + show.link, m_headers).body);
        if (tip) {
            auto syn = tip.selectFirst("//div[contains(@class,'synopsis')]");
            if (syn) show.description = syn.text().trimmed();
            auto status = tip.selectFirst("//div[span[contains(text(),'Status')]]/span[2]");
            if (status) show.status = status.text().trimmed();
            auto year = tip.selectFirst("//div[span[contains(text(),'Year')]]/span[2]");
            if (year) show.releaseDate = year.text().trimmed();
            auto score = tip.selectFirst("//div[span[contains(text(),'Scores')]]/span[2]");
            if (score) show.score = score.text().trimmed();
            auto genreLinks = tip.select("//div[span[contains(text(),'Genre')]]//a");
            for (const auto &g : std::as_const(genreLinks)) {
                QString gt = g.text().trimmed();
                if (!gt.isEmpty()) show.genres.push_back(gt);
            }
        }
    }
    return eps.size();
}

QList<VideoServer> Anikoto::loadServers(Client *client, const PlaylistItem *episode) const {
    QList<VideoServer> servers;
    QString url = hostUrl() + "ajax/server/list?servers=" + QUrl::toPercentEncoding(episode->link);
    QString html = client->get(url, m_headers).toJsonObject().value("result").toString();
    auto doc = CSoup::parse(html);
    if (!doc) return servers;

    auto typeDivs = doc.select("//div[contains(@class,'type') and @data-type]");
    for (const auto &td : std::as_const(typeDivs)) {
        bool isDub = td.attr("data-type") == "dub";
        QString suffix = isDub ? " Dub" : " Sub";
        auto tr = isDub ? VideoServer::Dub : VideoServer::Sub;
        auto lis = td.select(".//li[@data-link-id]");
        for (const auto &li : std::as_const(lis)) {
            QString linkId = li.attr("data-link-id");
            if (linkId.isEmpty()) continue;
            servers.emplaceBack(li.text().trimmed() + suffix, linkId, tr);
        }
    }
    return servers;
}

PlayInfo Anikoto::extractSource(Client *client, VideoServer server) {
    // data-link-id -> the actual embed page url (vidtube / megaplay / vidwish / ...).
    QString getUrl = hostUrl() + "ajax/server?get=" + QUrl::toPercentEncoding(server.link);
    QJsonObject result = client->get(getUrl, m_headers).toJsonObject().value("result").toObject();
    QString embedUrl = result.value("url").toString();
    if (embedUrl.isEmpty()) return {};
    return extractEmbed(client, embedUrl, server);
}

PlayInfo Anikoto::extractEmbed(Client *client, const QString &embedUrl, const VideoServer &server) const {
    PlayInfo info;
    QUrl u(embedUrl);
    QString origin = u.scheme() + "://" + u.host();
    QString type = server.translation == VideoServer::Dub ? "dub" : "sub";

    QMap<QString, QString> pageHeaders = m_headers;
    pageHeaders["Referer"] = hostUrl();
    QString page = client->get(embedUrl, pageHeaders).body;

    // These embeds are all megaplay/megacloud-family: the player id lives on #megaplay-player.
    static const QRegularExpression idRe(R"RX(id="megaplay-player"[^>]*?data-id="(\d+)")RX");
    auto m = idRe.match(page);
    QString id = m.hasMatch() ? m.captured(1) : QString();
    if (id.isEmpty()) {
        static const QRegularExpression anyId(R"RX(data-id="(\d+)")RX");
        auto m2 = anyId.match(page);
        if (m2.hasMatch()) id = m2.captured(1);
    }
    if (id.isEmpty()) return info;

    QMap<QString, QString> srcHeaders = m_headers;
    srcHeaders["Referer"] = embedUrl;
    // Most hosts expose getSourcesNew; vidwish-style hosts use getSources. Try both.
    for (const QString &endpoint : {QStringLiteral("stream/getSourcesNew"), QStringLiteral("stream/getSources")}) {
        QString srcUrl = origin + "/" + endpoint + "?id=" + id + "&type=" + type;
        QJsonObject json = client->get(srcUrl, srcHeaders).toJsonObject();
        QString file = json.value("sources").toObject().value("file").toString();
        if (file.isEmpty()) continue;

        for (const QJsonValue &t : json.value("tracks").toArray()) {
            QJsonObject to = t.toObject();
            if (to.value("kind").toString() != "captions") continue;
            QString subUrl = to.value("file").toString();
            if (!subUrl.isEmpty())
                info.subtitles.emplaceBack(QUrl(subUrl), to.value("label").toString());
        }
        // The subtitle host 403s without the embed origin as Referer (the video goes via the proxy,
        // which carries its own Referer). PNG-wrapped segments are de-obfuscated by the proxy.
        info.addHeader("Referer", origin + "/");
        info.addHeader("User-Agent", m_headers["User-Agent"]);

        HlsProxy *proxy = HlsProxy::instance();
        info.videos.emplaceBack(QUrl(proxy ? proxy->playlistUrl(file, origin + "/") : file), server.name);
        break;
    }
    return info;
}
