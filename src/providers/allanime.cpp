#include "providers/allanime.h"
#include "app/settings.h"
#include "providers/jsunpack.h"
#include <QFileInfo>
#include <QMap>
#include <QSet>
#include <QUrl>
#include <QCryptographicHash>
#include <QRegularExpression>
#include "providers/aes.h"
#include "providers/providerregistry.h"

REGISTER_PROVIDER(AllAnime, 5)

namespace {
constexpr const char *kSearchHash  = "a24c500a1b765c68ae1d8dd85174931f661c71369c89b92b88b75a725afc471c";
constexpr const char *kPopularHash = "60f50b84bb545fa25ee7f7c8c0adbf8f5cea40f7b1ef8501cbbff70e38589489";
constexpr const char *kShowHash    = "043448386c7a686bc2aabfbb6b80f6074e795d350df48015023b079527b0848a";
constexpr const char *kEpisodeHash = "d405d0edd690624b66baba3068e0edc3ac90f1597d898a1ec8db4e5c43c00fec";

QByteArray b64urlDecode(const QString &s) {
    return QByteArray::fromBase64(s.toLatin1(), QByteArray::Base64UrlEncoding);
}

// Filemoon (Fm-mp4): endpoint returns AES-256-CTR JSON {iv, payload, key_parts} of quality URLs.
void extractFilemoon(const QJsonObject &json, PlayInfo &playItem) {
    const auto keyParts = json["key_parts"].toArray();
    if (keyParts.size() < 2 || !json.contains("payload")) return;

    QByteArray key     = b64urlDecode(keyParts[0].toString()) + b64urlDecode(keyParts[1].toString());
    QByteArray counter = b64urlDecode(json["iv"].toString());
    counter.append(QByteArray::fromHex("00000002"));
    QByteArray payload = b64urlDecode(json["payload"].toString());
    if (key.size() != 32 || counter.size() != 16 || payload.size() <= 16) return;
    payload.chop(16);   // drop the trailing auth tag (CTR ignores it)

    QString text = QString::fromUtf8(Aes::ctr(key, counter, payload));
    if (text.isEmpty()) return;
    text.replace("\\u0026", "&").replace("\\u003D", "=").replace("\\/", "/");

    static const QRegularExpression re(
        R"RX("url":"([^"]+)"[^}]*?"height":(\d+)|"height":(\d+)[^}]*?"url":"([^"]+)")RX");
    auto it = re.globalMatch(text);
    while (it.hasNext()) {
        const auto m = it.next();
        const QString url = !m.captured(1).isEmpty() ? m.captured(1) : m.captured(4);
        const QString h   = !m.captured(2).isEmpty() ? m.captured(2) : m.captured(3);
        if (!url.isEmpty())
            playItem.videos.emplaceBack(url, h.isEmpty() ? QString() : h + "p", h.toInt());
    }
}

// ok.ru: data-options -> flashvars.metadata holds an HLS manifest + mp4 fallbacks.
// Signed URLs are IP+UA-bound, so the same Gecko UA is forwarded to the player.
void parseOkRu(const QString &page, PlayInfo &playItem, const QString &userAgent) {
    static const QRegularExpression re(QStringLiteral("data-options=\"([^\"]+)\""));
    const auto m = re.match(page);
    if (!m.hasMatch()) return;

    QString opts = m.captured(1);
    opts.replace("&quot;", "\"").replace("&amp;", "&").replace("&#39;", "'")
        .replace("&lt;", "<").replace("&gt;", ">");

    const QJsonObject flashvars = QJsonDocument::fromJson(opts.toUtf8())
                                      .object().value("flashvars").toObject();
    const QJsonValue metaVal = flashvars.value("metadata");
    const QJsonObject meta = metaVal.isString()
        ? QJsonDocument::fromJson(metaVal.toString().toUtf8()).object()
        : metaVal.toObject();
    if (meta.isEmpty()) return;

    // HLS field name varies (hlsManifestUrl/ondemandHls/...); prefer a manifest, else progressive.
    QString hls = meta.value("hlsManifestUrl").toString();
    if (hls.isEmpty()) hls = meta.value("ondemandHls").toString();
    if (hls.isEmpty()) hls = meta.value("hlsMasterPlaylistUrl").toString();
    if (!hls.isEmpty()) {
        playItem.videos.emplaceBack(hls);
    } else {
        for (const QJsonValue &v : meta.value("videos").toArray()) {  // progressive fallback
            const QJsonObject o = v.toObject();
            const QString url = o.value("url").toString();
            if (!url.isEmpty()) playItem.videos.emplaceBack(url, o.value("name").toString());
        }
    }
    if (!playItem.videos.isEmpty() && !userAgent.isEmpty())
        playItem.addHeader("user-agent", userAgent);
}

// byse (Fm-Hls): /api/videos/<code>/ -> {version, key_parts[], iv, payload} (AES-256-GCM).
// Real key = key_parts[version] + key_parts[31-version] (rest are decoys).
void parseByse(const QJsonObject &resp, PlayInfo &playItem, const QString &userAgent) {
    const QJsonObject pb = resp.value("playback").toObject();
    const QJsonArray keyParts = pb.value("key_parts").toArray();
    const int n = keyParts.size();
    if (n == 0) return;

    const int version = pb.value("version").toString().toInt();
    const int i = version, s = 31 - version;
    QByteArray key;
    if (i >= 1 && i <= n && s >= 1 && s <= n) {
        key = b64urlDecode(keyParts[i - 1].toString()) + b64urlDecode(keyParts[s - 1].toString());
    } else {
        for (const QJsonValue &p : keyParts) key += b64urlDecode(p.toString());
    }

    const QByteArray plain = Aes::gcmDecrypt(key, b64urlDecode(pb.value("iv").toString()),
                                                b64urlDecode(pb.value("payload").toString()));
    if (plain.isEmpty()) return;

    const QJsonObject obj = QJsonDocument::fromJson(plain).object();
    for (const QJsonValue &v : obj.value("sources").toArray()) {
        const QJsonObject src = v.toObject();
        const QString url = src.value("url").toString();
        if (!url.isEmpty())
            playItem.videos.emplaceBack(url, src.value("label").toString(), src.value("height").toInt());
    }
    for (const QJsonValue &v : obj.value("tracks").toArray()) {
        const QJsonObject tr = v.toObject();
        const QString url = tr.value("url").toString();
        if (!url.isEmpty()) playItem.subtitles.emplaceBack(url, tr.value("title").toString());
    }
    if (!playItem.videos.isEmpty() && !userAgent.isEmpty())
        playItem.addHeader("user-agent", userAgent);
}
}

QList<ShowData> AllAnime::search(Client *client, const QString &query, int page, int type) {
    Q_UNUSED(type);
    QString variables = QString(
                            "{%22search%22:{%22query%22:%22%1%22},%22limit%22:26,%22page%22:%2"
                            ",%22translationType%22:%22sub%22,%22countryOrigin%22:%22ALL%22}")
                            .arg(QUrl::toPercentEncoding(query), QString::number(page));

    auto data = client->get(apiUrl(variables, kSearchHash), m_headers)
                    .toJsonObject()["data"].toObject();
    return parseJsonArray(data["shows"].toObject()["edges"].toArray());
}

QList<ShowData> AllAnime::popular(Client *client, int page, int typeIndex) {
    Q_UNUSED(typeIndex);
    QString variables = QString(
                            "{%22type%22:%22anime%22,%22size%22:20,%22dateRange%22:0,%22page%22:%1"
                            ",%22allowAdult%22:false,%22allowUnknown%22:false}")
                            .arg(page);

    auto data = client->get(apiUrl(variables, kPopularHash), m_headers)
                    .toJsonObject()["data"].toObject();
    return parseJsonArray(data["queryPopular"].toObject()["recommendations"].toArray(), true);
}

QList<ShowData> AllAnime::latest(Client *client, int page, int typeIndex) {
    Q_UNUSED(typeIndex);
    QString variables = QString(
                            "{%22search%22:{},%22limit%22:26,%22page%22:%1"
                            ",%22translationType%22:%22sub%22,%22countryOrigin%22:%22JP%22}")
                            .arg(page);

    auto data = client->get(apiUrl(variables, kSearchHash), m_headers)
                    .toJsonObject()["data"].toObject();
    return parseJsonArray(data["shows"].toObject()["edges"].toArray());
}

QList<ShowData> AllAnime::parseJsonArray(const QJsonArray &shows, bool isPopular) {
    QList<ShowData> results;
    results.reserve(shows.size());

    for (const QJsonValue &val : shows) {
        QJsonObject item = val.toObject();
        if (item.isEmpty()) continue;
        if (isPopular) item = item["anyCard"].toObject();

        QString title = item["name"].toString();
        QString link = item["_id"].toString();
        if (title.isEmpty() && link.isEmpty()) continue;

        results.emplaceBack(title, link, getCoverImage(item), this, "", ShowData::ANIME);
    }
    return results;
}

int AllAnime::loadShow(Client *client, ShowData &show, bool getEpisodeCountOnly, bool getPlaylist, bool getInfo) const {
    QString variables = QString("{%22_id%22:%22%1%22}").arg(show.link);
    auto json = client->get(apiUrl(variables, kShowHash), m_headers)
                    .toJsonObject()["data"].toObject()["show"].toObject();
    if (json.isEmpty()) return 0;

    QJsonArray subEps = json["availableEpisodesDetail"].toObject()["sub"].toArray();
    QJsonArray dubEps = json["availableEpisodesDetail"].toObject()["dub"].toArray();

    // Count must match the sub + dub merge below, else the unwatched badge breaks.
    if (getEpisodeCountOnly) {
        QSet<float> unique;
        unique.reserve(subEps.size() + dubEps.size());
        for (const QJsonValue &v : std::as_const(subEps)) unique.insert(v.toString().toFloat());
        for (const QJsonValue &v : std::as_const(dubEps)) unique.insert(v.toString().toFloat());
        return unique.size();
    }

    if (getPlaylist) {
        QMap<float, QPair<QString, QString>> episodeMap;

        for (const QJsonValue &v : std::as_const(subEps)) {
            QString ep = v.toString();
            float num = ep.toFloat();
            episodeMap[num].first = QString(R"({"showId":"%1","translationType":"sub","episodeString":"%2"})").arg(show.link, ep);
        }
        for (const QJsonValue &v : std::as_const(dubEps)) {
            QString ep = v.toString();
            float num = ep.toFloat();
            episodeMap[num].second = QString(R"({"showId":"%1","translationType":"dub","episodeString":"%2"})").arg(show.link, ep);
        }

        for (auto it = episodeMap.constBegin(); it != episodeMap.constEnd(); ++it) {
            const auto &[sub, dub] = it.value();
            QString vars;
            if (!sub.isEmpty()) vars = sub;
            if (!dub.isEmpty()) {
                if (!vars.isEmpty()) vars += ";";
                vars += dub;
            }
            show.addEpisode(0, it.key(), vars, "");
        }
    }

    if (!getInfo) return subEps.size();

    show.description = json["description"].toString();
    show.status = json["status"].toString();
    show.views = json["pageStatus"].toObject()["views"].toString();
    show.coverUrl = getCoverImage(json);

    QJsonValue malScore = json["score"];
    QJsonValue aniScore = json["averageScore"];
    QStringList scores;
    if (!malScore.isUndefined() && !malScore.isNull())
        scores << QString::number(malScore.toDouble(), 'f', 1) + " (MAL)";
    if (!aniScore.isUndefined() && !aniScore.isNull())
        scores << QString::number(aniScore.toInt()) + " (Anilist)";
    show.score = scores.join("; ");

    for (const QJsonValue &g : json["genres"].toArray())
        show.genres.push_back(g.toString());

    QJsonObject aired = json["airedStart"].toObject();
    int day   = aired["date"].toInt(-1);
    int month = aired["month"].toInt(-1) + 1;
    int year  = aired["year"].toInt(-1);
    QDate airedDate(year, month, day);
    if (airedDate.isValid()) {
        show.releaseDate = airedDate.toString("MMMM d, yyyy");
        show.updateTime = airedDate.toString("'Every' dddd");
    }
    if (aired.contains("hour")) {
        int hour = aired["hour"].toInt();
        int minute = aired["minute"].toInt(0);
        show.updateTime += QString(" at %1:%2")
                               .arg(hour, 2, 10, QLatin1Char('0'))
                               .arg(minute, 2, 10, QLatin1Char('0'));
    }

    return subEps.size();
}

QList<VideoServer> AllAnime::loadServers(Client *client, const PlaylistItem *episode) const {
    QList<VideoServer> servers;
    const auto subDubVariables = episode->link.split(";");

    for (int i = 0; i < subDubVariables.size(); ++i) {
        const QString &vars = subDubVariables[i];
        QString url = apiUrl(QUrl::toPercentEncoding(vars), kEpisodeHash);
        auto data = client->get(url, m_headers).toJsonObject()["data"].toObject();

        if (data.contains("tobeparsed")) {
            data = decryptTobeParsed(data["tobeparsed"].toString());
        }

        auto sourceUrls = data["episode"].toObject()["sourceUrls"].toArray();
        bool isSub = (i == 0);
        QString suffix = isSub ? " Sub" : " Dub";
        auto tr = isSub ? VideoServer::Sub : VideoServer::Dub;
        for (const QJsonValue &val : std::as_const(sourceUrls)) {
            QJsonObject src = val.toObject();
            servers.emplaceBack(src["sourceName"].toString() + suffix, src["sourceUrl"].toString(), tr);
        }
    }

    std::sort(servers.begin(), servers.end(),
              [](const VideoServer &a, const VideoServer &b) {
                  if (a.translation != b.translation) return a.translation < b.translation;
                  return a.name < b.name;
              });
    return servers;
}

// AES-256-GCM. Payload = base64(version[1]|IV[12]|ciphertext|tag[16]), key = SHA-256("Xot36i3lK3:v1").
QJsonObject AllAnime::decryptTobeParsed(const QString &payload) {
    QByteArray raw = QByteArray::fromBase64(payload.toLatin1());
    constexpr int versionLen = 1, ivLen = 12, tagLen = 16;
    if (raw.size() <= versionLen + ivLen + tagLen) return {};

    if (static_cast<quint8>(raw.at(0)) != 1) return {};

    static const QByteArray key = [] {
        return QCryptographicHash::hash(QByteArrayLiteral("Xot36i3lK3:v1"),
                                        QCryptographicHash::Sha256);
    }();

    const QByteArray plaintext = Aes::gcmDecrypt(key, raw.mid(versionLen, ivLen),
                                                       raw.mid(versionLen + ivLen));
    if (plaintext.isEmpty()) return {};

    QJsonParseError err;
    auto doc = QJsonDocument::fromJson(plaintext, &err);
    return err.error == QJsonParseError::NoError ? doc.object() : QJsonObject{};
}

PlayInfo AllAnime::extractSource(Client *client, VideoServer server) {
    PlayInfo playItem;
    auto decryptedLink = decryptSource(server.link);

    if (server.name.startsWith("Mp4")) {
        auto response = client->get(decryptedLink, m_headers).body;
        static QRegularExpression regex(R"(src: "([^"]+))");
        auto match = regex.match(response);
        if (match.hasMatch()) {
            playItem.videos.emplaceBack(match.captured(1));
            playItem.addHeader("referer", "https://mp4upload.com/");
            playItem.addHeader("user-agent", m_headers["User-Agent"]);
        }
    }
    else if (server.name.startsWith("Fm-Hls")) {
        // byse hosts (bysekoze.com etc.): /api/videos/<code>/ -> AES-256-GCM config.
        const QUrl embedUrl(server.link);
        const QString code = embedUrl.path().section('/', -1, -1, QString::SectionSkipEmpty);
        if (!code.isEmpty()) {
            const QString api = QString("%1://%2/api/videos/%3/")
                                    .arg(embedUrl.scheme(), embedUrl.host(), code);
            const QJsonObject resp = client->get(api, m_headers).toJsonObject();
            if (resp.contains("playback")) {
                parseByse(resp, playItem, m_headers["User-Agent"]);
                // byse CDN rejects requests without a Referer.
                if (!playItem.videos.isEmpty())
                    playItem.addHeader("referer", QString("%1://%2/").arg(embedUrl.scheme(), embedUrl.host()));
                return playItem;
            }
        }
        // Legacy iframe embed fallback (older Fm-Hls hosts) - packed-JS player.
        const QString body = client->get(server.link).body;
        int p = body.indexOf("iframe src=\"");
        if (p < 0) return playItem;
        const QString iframeSrc = body.mid(p + 12).section('"', 0, 0);
        if (!iframeSrc.startsWith("http")) return playItem;
        const QString unpacked = Js::unpack(client->get(iframeSrc).body);
        static const QRegularExpression re(R"(:\[\{file:"([^"]+)\")");
        auto m = re.match(unpacked);
        if (m.hasMatch()) playItem.videos.emplaceBack(m.captured(1));
    }
    else if (server.name.startsWith("Sw")) {
        // streamwish & clones: packed-JS embed holds sources:[{file:"<m3u8>"}].
        const QString html = client->get(decryptedLink, m_headers).body;
        QString hay = Js::unpack(html);
        if (hay.isEmpty()) hay = html;
        static const QRegularExpression re(QStringLiteral("file:\"(https?://[^\"]+\\.m3u8[^\"]*)\""));
        auto m = re.match(hay);
        if (m.hasMatch()) {
            playItem.videos.emplaceBack(m.captured(1));
            const QUrl u(decryptedLink);
            playItem.addHeader("referer", QString("%1://%2/").arg(u.scheme(), u.host()));
            playItem.addHeader("user-agent", m_headers["User-Agent"]);
        }
    }
    else if (server.name.startsWith("Sl")) {
        // streamlare: POST /api/video/stream/get -> result.<quality>.file (HLS/mp4).
        const QUrl u(decryptedLink);
        const QString id = u.path().section('/', -1, -1, QString::SectionSkipEmpty);
        if (!id.isEmpty()) {
            QMap<QString, QString> headers = m_headers;
            headers["Content-Type"] = "application/json";
            headers["Referer"] = decryptedLink;
            const QByteArray body = QByteArray("{\"id\":\"") + id.toUtf8() + "\"}";
            const QString api = QString("%1://%2/api/video/stream/get").arg(u.scheme(), u.host());
            const QJsonObject result = client->post(api, body, headers).toJsonObject()
                                           .value("result").toObject();
            for (auto it = result.constBegin(); it != result.constEnd(); ++it) {
                const QString file = it.value().toObject().value("file").toString();
                if (!file.isEmpty()) { playItem.videos.emplaceBack(file, it.key()); break; }
            }
            if (!playItem.videos.isEmpty())
                playItem.addHeader("user-agent", m_headers["User-Agent"]);
        }
    }
    else if (server.name.startsWith("Yt") && decryptedLink.startsWith("http")) {
        // Yt-mp4 direct; a clock-path Yt falls through to /apivtwo below.
        playItem.videos.emplaceBack(decryptedLink);
        playItem.addHeader("referer", "https://allanime.day/");
        playItem.addHeader("user-agent", m_headers["User-Agent"]);
    }
    else if (server.name.startsWith("Ok")) {
        parseOkRu(client->get(decryptedLink, m_headers).body, playItem, m_headers["User-Agent"]);
    }
    else if (server.name.startsWith("Vn")) {
        // vidnest (XFileSharing): POST /dl -> JWPlayer config with direct mp4 sources.
        const QUrl embedUrl(server.link);
        const QString code = embedUrl.path().section('/', -1, -1, QString::SectionSkipEmpty);
        if (!code.isEmpty()) {
            const QString dl = QString("%1://%2/dl").arg(embedUrl.scheme(), embedUrl.host());
            const QMap<QString, QString> form{
                {"op", "embed"}, {"file_code", code}, {"auto", "1"}, {"referer", kEndPoint}};
            QMap<QString, QString> headers = m_headers;
            headers["Referer"] = server.link;
            const QString page = client->post(dl, form, headers).body;

            static const QRegularExpression srcRe(R"RX(file:"([^"]+)"(?:,label:"([^"]*)")?)RX");
            static const QRegularExpression hRe(QStringLiteral("x(\\d+)"));
            auto it = srcRe.globalMatch(page);
            while (it.hasNext()) {
                const auto m = it.next();
                const QString url = m.captured(1);
                if (!url.startsWith("http") || (!url.contains(".mp4") && !url.contains(".m3u8")))
                    continue;
                const QString label = m.captured(2);
                const auto hm = hRe.match(label);
                playItem.videos.emplaceBack(url, label, hm.hasMatch() ? hm.captured(1).toInt() : 0);
            }
            if (!playItem.videos.isEmpty()) {
                playItem.addHeader("user-agent", m_headers["User-Agent"]);
                playItem.addHeader("referer", QString("%1://%2/").arg(embedUrl.scheme(), embedUrl.host()));
            }
        }
    }
    else if (server.name.startsWith("Fm-mp4")) {
        QString path = decryptedLink;
        if (path.contains("/clock") && !path.contains("/clock.json"))
            path.replace("/clock", "/clock.json");
        QString url = path.startsWith("http") ? path : QString(kEndPoint) + path;
        extractFilemoon(client->get(url, m_headers).toJsonObject(), playItem);
    }
    else if (decryptedLink.startsWith("/apivtwo")) {
        auto url = QString(kEndPoint) + decryptedLink.insert(14, ".json");
        auto response = client->get(url, m_headers).toJsonObject();
        auto links = response["links"].toArray();

        for (const QJsonValue &linkVal : std::as_const(links)) {
            QJsonObject link = linkVal.toObject();

            if (link.contains("dash")) {
                auto rawUrls = link["rawUrls"].toObject();
                for (const QJsonValue &v : rawUrls["vids"].toArray()) {
                    QJsonObject vid = v.toObject();
                    int h = vid["height"].toInt();
                    int bw = vid["bandwidth"].toInt();
                    QString label = QString("%1p %2").arg(h).arg(Track::formatBitrate(bw));
                    playItem.videos.emplaceBack(vid["url"].toString(), label, h, bw);
                }
                for (const QJsonValue &a : rawUrls["audios"].toArray()) {
                    QJsonObject aud = a.toObject();
                    int bw = aud["bandwidth"].toInt();
                    playItem.audios.emplaceBack(aud["url"].toString(), Track::formatBitrate(bw), "", bw);
                }
            } else {
                playItem.videos.emplaceBack(link["link"].toString());
            }

            if (!link.contains("subtitles")) continue;
            for (const QJsonValue &s : link["subtitles"].toArray()) {
                QJsonObject sub = s.toObject();
                QString subUrl = sub["src"].toString();
                QString label = sub["label"].toString();

                if (subUrl.startsWith("https://allanime.pro/apiak/sk.json")) {
                    auto subResp = client->get(subUrl);
                    if (subResp.body.startsWith("{\"font")) {
                        const QString path = convertJsonSubToSrt(subResp.toJsonObject(), subUrl);
                        if (path.isEmpty()) continue;
                        // A bare path parses its drive letter as the scheme.
                        playItem.subtitles.emplaceBack(QUrl::fromLocalFile(path), label);
                        continue;
                    }
                }
                playItem.subtitles.emplaceBack(subUrl, label);
            }
        }

        // Some sources expose only a top-level HLS stream, not per-quality links.
        auto hls = response["hls"].toObject();
        if (hls.contains("url"))
            playItem.videos.emplaceBack(hls["url"].toString());
    }
    return playItem;
}

QString AllAnime::getCoverImage(const QJsonObject &json) const {
    QString url = json["thumbnail"].toString();
    if (!url.startsWith("https"))
        url = "https://wp.youtube-anime.com/aln.youtube-anime.com/" + url;
    return url + "?w=250";
}

QString AllAnime::decryptSource(const QString &input) const {
    if (!input.startsWith('-')) return input;

    QString hexString = input.section('-', -1);
    QString result;
    result.reserve(hexString.length() / 2);
    for (int i = 0; i + 1 < hexString.length(); i += 2) {
        bool ok;
        int byte = hexString.mid(i, 2).toInt(&ok, 16);
        if (ok) result += QChar(byte ^ 56);
    }
    return result;
}

QString AllAnime::convertJsonSubToSrt(const QJsonObject &json, const QString &sourceUrl) const {
    QString fileName = sourceUrl.split("?").last();
    QString filePath = Settings::getTempDir() + "/" + fileName;

    QFile outputFile(filePath);
    if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        oLog() << name() << "Failed to write subtitle file:" << filePath;
        return {};
    }

    QTextStream out(&outputFile);
    int index = 1;
    for (const QJsonValue &val : json["body"].toArray()) {
        QJsonObject line = val.toObject();
        double from = line["from"].toDouble();
        double to = line["to"].toDouble();
        QString content = line["content"].toString();

        out << index++ << "\n"
            << msToSrtTime(from) << " --> " << msToSrtTime(to) << "\n"
            << content << "\n\n";
    }
    outputFile.close();
    return QFileInfo(outputFile).absoluteFilePath();
}