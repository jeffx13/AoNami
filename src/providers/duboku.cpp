#include "providers/duboku.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QUrl>
#include "providers/providerregistry.h"

REGISTER_PROVIDER(Duboku, 6)

// Listing paths carry 12 dash-separated fields (type at 0, sort at 2, page at 8); search paths
// carry 14 (keyword at 0, page at 10). Both layouts tag their cards with the class `module-item`.

QString Duboku::absolute(const QString &path) const {
    if (path.isEmpty() || path.startsWith("http")) return path;
    return hostUrl() + (path.startsWith('/') ? path.mid(1) : path);
}

int Duboku::showTypeOf(const QString &label) {
    if (label.contains("动漫")) return ShowData::ANIME;
    if (label.contains("电影")) return ShowData::MOVIE;
    if (label.contains("剧")) return ShowData::TVSERIES;
    if (label.contains("综艺")) return ShowData::VARIETY;
    return ShowData::NONE;
}

QList<ShowData> Duboku::parseList(const QString &html, int showType) {
    QList<ShowData> shows;
    auto doc = Html::parse(html);
    if (!doc) return shows;

    auto items = doc.select("//*[contains(concat(' ', normalize-space(@class), ' '), ' module-item ')]");
    for (const auto &item : std::as_const(items)) {
        QString href = item.attr("href");
        if (href.isEmpty()) {
            auto a = item.selectFirst(".//a[contains(@href,'/v/')]");
            if (a) href = a.attr("href");
        }
        static const QRegularExpression idRe(R"(/v/(\d+)\.html)");
        auto m = idRe.match(href);
        if (!m.hasMatch()) continue;

        QString title = item.attr("title");
        if (title.isEmpty()) {
            auto t = item.selectFirst(".//*[contains(@class,'-item-title')]");
            if (t) title = t.text().simplified();
        }
        if (title.isEmpty()) continue;

        QString cover;
        if (auto img = item.selectFirst(".//img")) cover = absolute(img.attr("data-original"));

        QString note;
        if (auto n = item.selectFirst(".//*[contains(@class,'module-item-note')]")) note = n.text().simplified();

        int type = showType;
        if (auto c = item.selectFirst(".//*[contains(@class,'module-card-item-class')]"))
            type = showTypeOf(c.text().simplified());

        shows.emplaceBack(title, m.captured(1), cover, this, note, type);
    }
    return shows;
}

QList<ShowData> Duboku::listing(Client *client, int page, int typeIndex, const QString &by) {
    const int i = qBound(0, typeIndex, int(std::size(kTypeIds)) - 1);
    const QString url = hostUrl() + QString("k/%1--%2------%3---.html")
                                        .arg(kTypeIds[i]).arg(by).arg(qMax(1, page));
    static constexpr int kShowTypes[] = {ShowData::ANIME, ShowData::MOVIE,
                                         ShowData::TVSERIES, ShowData::VARIETY};
    return parseList(client->get(url, m_headers).body, kShowTypes[i]);
}

QList<ShowData> Duboku::popular(Client *client, int page, int typeIndex) {
    return listing(client, page, typeIndex, QStringLiteral("hits"));
}

QList<ShowData> Duboku::latest(Client *client, int page, int typeIndex) {
    return listing(client, page, typeIndex, QStringLiteral("time"));
}

QList<ShowData> Duboku::search(Client *client, const QString &query, int page, int type) {
    Q_UNUSED(type)
    if (query.trimmed().isEmpty()) return {};
    const QString url = hostUrl() + QString("s/%1----------%2---.html")
                                        .arg(QString::fromUtf8(QUrl::toPercentEncoding(query.trimmed())))
                                        .arg(qMax(1, page));
    return parseList(client->get(url, m_headers).body, ShowData::NONE);
}

int Duboku::loadShow(Client *client, ShowData &show, bool getEpisodeCountOnly, bool getPlaylist, bool getInfo) const {
    const QString html = client->get(hostUrl() + "v/" + show.link + ".html", m_headers).body;
    auto doc = Html::parse(html);
    if (!doc) return 0;

    auto lists = doc.select("//*[contains(@class,'module-play-list-content')]");
    if (lists.isEmpty()) return 0;

    // Sources lag behind each other, so the longest list is the most complete one.
    QVector<Html::Node> episodes;
    for (const auto &list : std::as_const(lists)) {
        auto eps = list.select(".//a[contains(@href,'/p/')]");
        if (eps.size() > episodes.size()) episodes = std::move(eps);
    }

    if (getEpisodeCountOnly && !getPlaylist && !getInfo) return episodes.size();

    if (getPlaylist) {
        static const QRegularExpression epRe(R"(/p/([\d-]+)\.html)");
        for (const auto &ep : std::as_const(episodes)) {
            auto m = epRe.match(ep.attr("href"));
            if (!m.hasMatch()) continue;
            QString title = ep.text().simplified();
            const float number = resolveTitleNumber(title);
            show.addEpisode(0, number, m.captured(1), number < 0 ? title : QString());
        }
    }

    if (getInfo) {
        if (auto d = doc.selectFirst("//*[contains(@class,'module-info-introduction-content')]"))
            show.description = d.text().simplified();

        auto field = [&doc](const QString &label) -> QString {
            auto n = doc.selectFirst("//*[contains(@class,'module-info-item')]"
                                     "[span[contains(text(),'" + label + "')]]"
                                     "/*[contains(@class,'module-info-item-content')]");
            return n ? n.text().simplified() : QString();
        };
        show.releaseDate = field("上映");
        show.updateTime  = field("更新");
        show.status      = field("连载");

        auto tags = doc.select("//*[contains(@class,'module-info-tag-link')]/a");
        for (const auto &t : std::as_const(tags)) {
            const QString g = t.text().simplified();
            if (!g.isEmpty()) show.genres.push_back(g);
        }
    }
    return episodes.size();
}

QList<VideoServer> Duboku::loadServers(Client *client, const PlaylistItem *episode) const {
    QList<VideoServer> servers;
    const QStringList parts = episode->link.split('-');
    if (parts.size() != 3) return servers;
    const QString vodId = parts[0], nid = parts[2];

    auto doc = Html::parse(client->get(hostUrl() + "v/" + vodId + ".html", m_headers).body);
    if (!doc) return servers;

    auto names = doc.select("//*[@data-dropdown-value]");
    auto lists = doc.select("//*[contains(@class,'module-play-list-content')]");
    static const QRegularExpression sidRe(R"(/p/\d+-(\d+)-\d+\.html)");

    for (int i = 0; i < lists.size(); ++i) {
        auto first = lists[i].selectFirst(".//a[contains(@href,'/p/')]");
        if (!first) continue;
        auto m = sidRe.match(first.attr("href"));
        if (!m.hasMatch()) continue;
        const QString label = i < names.size() ? names[i].attr("data-dropdown-value").simplified() : QString();
        servers.emplaceBack(label.isEmpty() ? QString("线路 %1").arg(i + 1) : label,
                            vodId + "-" + m.captured(1) + "-" + nid);
    }
    return servers;
}

PlayInfo Duboku::extractSource(Client *client, VideoServer server) {
    PlayInfo info;
    if (server.link.isEmpty()) return info;

    const QString html = client->get(hostUrl() + "p/" + server.link + ".html", m_headers).body;
    static const QRegularExpression cfgRe(R"(player_aaaa\s*=\s*(\{.*?\})\s*</script>)",
                                          QRegularExpression::DotMatchesEverythingOption);
    auto m = cfgRe.match(html);
    if (!m.hasMatch()) return info;

    const QString url = QJsonDocument::fromJson(m.captured(1).toUtf8()).object().value("url").toString();
    if (url.isEmpty()) return info;

    info.videos.emplaceBack(QUrl(url), server.name);
    info.addHeader("Referer", hostUrl());
    info.addHeader("Origin", hostUrl().chopped(1));
    info.addHeader("User-Agent", m_headers.value("User-Agent"));
    return info;
}
