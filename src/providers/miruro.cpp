#include "miruro.h"
#include "registry.h"
#include "core/network/hlsproxy.h"
#include "app/logger.h"
#include <QJsonDocument>
#include <QRegularExpression>
#include <zlib.h>

REGISTER_PROVIDER(Miruro, 8)

namespace {

constexpr char kFallbackKey[] = "71951034f8fbcf53d89db52ceb3dc22c";
constexpr int  kPerPage = 28;

QByteArray gunzip(const QByteArray &in) {
    if (in.isEmpty()) return {};
    z_stream s{};
    if (inflateInit2(&s, 15 + 32) != Z_OK) return {};   // 32 = sniff gzip or zlib
    s.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(in.data()));
    s.avail_in = uInt(in.size());

    QByteArray out;
    char buf[32768];
    int rc = Z_OK;
    do {
        s.next_out = reinterpret_cast<Bytef *>(buf);
        s.avail_out = sizeof(buf);
        rc = inflate(&s, Z_NO_FLUSH);
        if (rc != Z_OK && rc != Z_STREAM_END && rc != Z_BUF_ERROR) break;
        out.append(buf, int(sizeof(buf) - s.avail_out));
    } while (rc != Z_STREAM_END && s.avail_in > 0);
    inflateEnd(&s);
    return rc == Z_STREAM_END ? out : QByteArray();
}

QString textOf(const QJsonObject &title) {
    for (const char *k : {"english", "romaji", "userPreferred", "native"})
        if (const QString v = title.value(QLatin1String(k)).toString(); !v.isEmpty()) return v;
    return {};
}

QString dateOf(const QJsonObject &d) {
    const int y = d.value("year").toInt();
    if (y <= 0) return {};
    const int m = d.value("month").toInt(), day = d.value("day").toInt();
    if (m <= 0)   return QString::number(y);
    if (day <= 0) return QString("%1-%2").arg(y).arg(m, 2, 10, QChar('0'));
    return QString("%1-%2-%3").arg(y).arg(m, 2, 10, QChar('0')).arg(day, 2, 10, QChar('0'));
}

// AniList descriptions arrive as HTML; the info page renders plain text.
QString stripTags(QString html) {
    static const QRegularExpression brRe(QStringLiteral("<br\\s*/?>"), QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression tagRe(QStringLiteral("<[^>]+>"));
    return html.replace(brRe, " ").remove(tagRe).simplified();
}

} // namespace

QByteArray Miruro::obfuscationKey(Client *client, bool refresh) const {
    {
        QMutexLocker lock(&m_cacheMutex);
        if (!refresh && !m_key.isEmpty()) return m_key;
    }

    QByteArray hex;
    const QString env = client->get(hostUrl() + "env2.js").body;
    static const QRegularExpression keyRe(QStringLiteral("VITE_PIPE_OBF_KEY[^0-9a-fA-F]+([0-9a-fA-F]{32})"));
    if (const auto m = keyRe.match(env); m.hasMatch()) hex = m.captured(1).toLatin1();
    if (hex.isEmpty()) hex = kFallbackKey;

    QMutexLocker lock(&m_cacheMutex);
    m_key = QByteArray::fromHex(hex);
    return m_key;
}

QJsonDocument Miruro::pipe(Client *client, const QString &path, const QJsonObject &query) const {
    const QJsonObject envelope{
        {"path", path}, {"method", "GET"}, {"query", query},
        {"body", QJsonValue::Null}, {"version", "0.2.0"},
    };
    const QByteArray e = QJsonDocument(envelope).toJson(QJsonDocument::Compact)
                             .toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);

    // The route only answers same-origin XHR, so it wants the browser's fetch headers.
    const QMap<QString, QString> headers{
        {"Referer", hostUrl()}, {"Origin", "https://www.miruro.to"},
        {"Accept", "*/*"}, {"Sec-Fetch-Dest", "empty"},
        {"Sec-Fetch-Mode", "cors"}, {"Sec-Fetch-Site", "same-origin"},
    };
    Client quiet = *client;
    quiet.setVerbose(false);
    const auto response = quiet.get(hostUrl() + "api/secure/pipe", headers, {{"e", QString::fromLatin1(e)}});

    QString label = path;
    if (const QString provider = query.value("provider").toString(); !provider.isEmpty())
        label += " " + provider + "/" + query.value("category").toString();
    if (response.code == 200) gLog() << "Miruro" << QString("(200) %1").arg(label);
    // 444 is how the edge says a backend has nothing for this episode.
    else                      oLog() << "Miruro" << QString("(%1) %2").arg(response.code).arg(label);

    if (response.code != 200 || response.body.isEmpty()) return {};

    // Plain JSON when the server didn't obfuscate; otherwise XOR then inflate.
    if (response.header("x-obfuscated").isEmpty())
        return QJsonDocument::fromJson(response.body.toUtf8());

    const QByteArray raw = QByteArray::fromBase64(response.body.toLatin1(), QByteArray::Base64UrlEncoding);
    for (int attempt = 0; attempt < 2; ++attempt) {
        const QByteArray key = obfuscationKey(client, attempt > 0);
        if (key.isEmpty()) break;

        QByteArray xored(raw.size(), Qt::Uninitialized);
        for (int i = 0; i < raw.size(); ++i)
            xored[i] = char(raw[i] ^ key[i % key.size()]);

        if (const QByteArray plain = gunzip(xored); !plain.isEmpty())
            return QJsonDocument::fromJson(plain);
    }

    oLog() << "Miruro" << "could not decode" << path;
    return {};
}

QJsonArray Miruro::browse(Client *client, const QJsonObject &query, QList<ShowData> &out) const {
    const QJsonArray results = pipe(client, "search", query).array();
    for (const QJsonValue &v : results) {
        const QJsonObject o = v.toObject();
        const int id = o.value("id").toInt();
        const QString title = textOf(o.value("title").toObject());
        if (id <= 0 || title.isEmpty()) continue;

        const QJsonObject cover = o.value("coverImage").toObject();
        QString art = cover.value("extraLarge").toString();
        if (art.isEmpty()) art = cover.value("large").toString();

        QString latest;
        if (const int eps = o.value("episodes").toInt(); eps > 0) latest = QString("%1 episodes").arg(eps);

        out.emplaceBack(title, QString::number(id), art, const_cast<Miruro *>(this), latest, ShowData::ANIME);
    }
    return results;
}

QList<ShowData> Miruro::search(Client *client, const QString &query, int page, int typeIndex) {
    Q_UNUSED(typeIndex);
    QList<ShowData> shows;
    if (query.trimmed().isEmpty()) return shows;
    browse(client, {{"q", query.trimmed()}, {"type", "ANIME"},
                    {"page", page}, {"perPage", kPerPage}}, shows);
    return shows;
}

QList<ShowData> Miruro::popular(Client *client, int page, int typeIndex) {
    Q_UNUSED(typeIndex);
    QList<ShowData> shows;
    browse(client, {{"type", "ANIME"}, {"sort", "POPULARITY_DESC"},
                    {"page", page}, {"perPage", kPerPage}}, shows);
    return shows;
}

QList<ShowData> Miruro::latest(Client *client, int page, int typeIndex) {
    Q_UNUSED(typeIndex);
    QList<ShowData> shows;
    // Not TRENDING_DESC - the edge drops that one with a 444.
    browse(client, {{"type", "ANIME"}, {"sort", "UPDATED_AT_DESC"},
                    {"page", page}, {"perPage", kPerPage}}, shows);
    return shows;
}

QJsonObject Miruro::episodesFor(Client *client, int anilistId) const {
    {
        QMutexLocker lock(&m_cacheMutex);
        if (const auto it = m_episodes.constFind(anilistId); it != m_episodes.constEnd()) return *it;
    }
    const QJsonObject data = pipe(client, "episodes", {{"anilistId", anilistId}}).object();
    if (data.isEmpty()) return {};

    QMutexLocker lock(&m_cacheMutex);
    if (m_episodes.size() > 8) m_episodes.clear();   // a long show runs to megabytes
    m_episodes.insert(anilistId, data);
    return data;
}

int Miruro::loadShow(Client *client, ShowData &show, bool getEpisodeCountOnly, bool getPlaylist, bool getInfo) const {
    const int anilistId = show.link.toInt();
    if (anilistId <= 0) return 0;

    if (getInfo) {
        const QJsonObject info = pipe(client, "info/anilist/" + show.link).object();
        if (!info.isEmpty()) {
            show.description = stripTags(info.value("description").toString());
            show.status      = info.value("status").toString();
            show.releaseDate = dateOf(info.value("startDate").toObject());
            if (const int score = info.value("averageScore").toInt(); score > 0)
                show.score = QString::number(score / 10.0, 'f', 1);
            for (const QJsonValue &g : info.value("genres").toArray())
                show.genres.push_back(g.toString());
            const QJsonObject cover = info.value("coverImage").toObject();
            if (const QString art = cover.value("extraLarge").toString(); !art.isEmpty())
                show.coverUrl = art;
        }
    }

    if (!getPlaylist && !getEpisodeCountOnly) return 0;

    const QJsonObject providers = episodesFor(client, anilistId).value("providers").toObject();
    if (providers.isEmpty()) return 0;

    // Providers disagree on how far they run, so the episode list is the union of
    // what they carry, keyed by episode number.
    QMap<double, QString> titles;
    for (const QJsonValue &pv : providers) {
        const QJsonObject byCategory = pv.toObject().value("episodes").toObject();
        for (const QJsonValue &lv : byCategory) {
            for (const QJsonValue &ev : lv.toArray()) {
                const QJsonObject ep = ev.toObject();
                const double number = ep.value("number").toDouble(-1);
                if (number < 0) continue;
                const QString title = ep.value("title").toString();
                if (!titles.contains(number) || titles[number].isEmpty()) titles.insert(number, title);
            }
        }
    }
    if (getEpisodeCountOnly || !getPlaylist) return int(titles.size());

    for (auto it = titles.constBegin(); it != titles.constEnd(); ++it) {
        QString name = it.value();
        if (name == QString("Episode %1").arg(it.key())) name.clear();   // the number already shows
        show.addEpisode(0, float(it.key()), QString("%1:%2").arg(anilistId).arg(it.key()), name);
    }
    return int(titles.size());
}

QList<VideoServer> Miruro::loadServers(Client *client, const PlaylistItem *episode) const {
    QList<VideoServer> servers;
    const QStringList parts = episode->link.split(':');
    if (parts.size() != 2) return servers;

    const int anilistId = parts[0].toInt();
    const double number = parts[1].toDouble();

    const QJsonObject providers = episodesFor(client, anilistId).value("providers").toObject();
    for (auto p = providers.constBegin(); p != providers.constEnd(); ++p) {
        const QJsonObject byCategory = p.value().toObject().value("episodes").toObject();
        for (auto c = byCategory.constBegin(); c != byCategory.constEnd(); ++c) {
            for (const QJsonValue &ev : c.value().toArray()) {
                const QJsonObject ep = ev.toObject();
                if (!qFuzzyCompare(ep.value("number").toDouble(-1) + 1.0, number + 1.0)) continue;
                const QString id = ep.value("id").toString();
                if (id.isEmpty()) break;

                const auto translation = c.key().startsWith("dub") ? VideoServer::Dub : VideoServer::Sub;
                servers.emplaceBack(p.key() + " · " + c.key(),
                                    QString("%1|%2|%3|%4").arg(id, p.key(), c.key()).arg(anilistId),
                                    translation);
                break;
            }
        }
    }
    return servers;
}

PlayInfo Miruro::extractSource(Client *client, VideoServer server) {
    PlayInfo info;
    const QStringList parts = server.link.split('|');
    if (parts.size() != 4) return info;

    const QJsonObject data = pipe(client, "sources", {
        {"episodeId", parts[0]}, {"provider", parts[1]},
        {"category", parts[2]}, {"anilistId", parts[3].toInt()},
    }).object();

    // Embeds would each need their own extractor; take the direct streams only,
    // preferring whichever the site marks as default.
    QJsonObject best;
    for (const QJsonValue &sv : data.value("streams").toArray()) {
        const QJsonObject s = sv.toObject();
        const QString type = s.value("type").toString();
        if (type != "hls" && type != "mp4") continue;
        if (best.isEmpty() || (s.value("default").toBool() && !best.value("default").toBool()))
            best = s;
    }
    if (best.isEmpty()) return info;

    QString label = best.value("server").toString();
    if (label.isEmpty()) label = server.name;

    // Several of these CDNs sit behind Cloudflare. Our own client clears them, so
    // checkVideo passes, but mpv has no jar and 403s - the server looks working and
    // then isn't. Serving the chain over loopback keeps the upstream fetch on our
    // side, exactly as kwik needs.
    const QString url = best.value("url").toString();
    const QString referer = best.value("referer").toString();
    QString playUrl = url;
    if (url.contains(".m3u8", Qt::CaseInsensitive))
        if (HlsProxy *proxy = HlsProxy::instance())
            playUrl = proxy->playlistUrl(url, referer);

    info.videos.emplaceBack(QUrl(playUrl), label);
    if (!referer.isEmpty()) info.addHeader("Referer", referer);

    for (const QJsonValue &sv : data.value("subtitles").toArray()) {
        const QJsonObject sub = sv.toObject();
        const QString file = sub.value("file").toString();
        if (file.isEmpty() || sub.value("kind").toString() == "thumbnails") continue;
        info.subtitles.emplaceBack(QUrl(file), sub.value("label").toString(), sub.value("language").toString());
    }
    return info;
}
