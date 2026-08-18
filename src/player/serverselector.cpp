#include "serverselector.h"
#include "providers/showprovider.h"
#include "core/network/cloudflare.h"
#include "app/logger.h"
#include "app/settings.h"
#include <QtConcurrent/QtConcurrentRun>
#include <atomic>
#include <mutex>
#include <algorithm>
#include <QUrl>
#include <QRegularExpression>
#include <QElapsedTimer>

namespace {

// code <= 0 is a transport blip; a real status, even 403, is an answer. Bypass off -
// a 403 from a dead CDN would open a browser and stall the race behind that solve.
// Clearances already held still go out.
Client::Response probe(Client *client, const QString &url,
                       QMap<QString, QString> headers, bool head,
                       const QString &range = {}) {
    if (!range.isEmpty()) headers.insert("Range", range);
    Client prober = *client;
    prober.setBypassEnabled(false);
    // HlsProxy holds the connection open while the upstream builds a playlist; anywhere
    // else, going quiet for that long means the host is gone.
    if (QUrl(url).host() == QLatin1String("127.0.0.1")) prober.setTimeout(45000);

    Client::Response response;
    QElapsedTimer timer;
    for (int attempt = 0; attempt < 2; ++attempt) {
        timer.start();
        response = head ? prober.head(url, headers) : prober.get(url, headers);
        // A failure that burned the full timeout means the host is gone, not blipping.
        if (response.code > 0 || prober.isCancelled() || timer.elapsed() > 2000) break;
    }
    return response;
}

}  // namespace

bool ServerSelector::checkVideo(Client *client, PlayInfo &playItem) {
    if (playItem.videos.isEmpty()) return false;
    const auto &video = playItem.videos.first();
    if (video.url.isLocalFile()) return true;

    const QString url = video.url.toString();

    // A stream can be challenged separately from the site that linked it, and mpv has no jar.
    Cloudflare::applyClearanceHeaders(video.url, playItem.headers);
    const auto &headers = playItem.headers;

    // A .m3u8 needs no sniffing and can take seconds to generate, so skip the HEAD.
    // Match the path, not the url: proxied ones carry the upstream in a query string.
    const bool looksHls = video.url.path().endsWith(QLatin1String(".m3u8"), Qt::CaseInsensitive);

    Client::Response headResp;
    QString contentType;
    if (!looksHls) {
        headResp = probe(client, url, headers, true);
        contentType = headResp.header("Content-Type").toLower();
    }
    Cloudflare::applyClearanceHeaders(video.url, playItem.headers);   // probe may have solved it

    // Many hosts refuse HEAD; settle it with a ranged GET before giving up.
    if (!looksHls && (headResp.code <= 0 || headResp.code == 405 || contentType.isEmpty())) {
        auto getResp = probe(client, url, headers, false, QStringLiteral("bytes=0-1023"));
        if (getResp.code <= 0 || getResp.code >= 400) return false;
        if (contentType.isEmpty()) contentType = getResp.header("Content-Type").toLower();
        if (headResp.code <= 0) headResp = getResp;
        if (getResp.body.startsWith(QLatin1String("#EXTM3U")))
            contentType = QStringLiteral("application/vnd.apple.mpegurl");
    }

    bool isHls = looksHls || contentType.contains("mpegurl");

    if (isHls) {
        // Probe down to a real segment - intact playlists with 404 segments fake "working".
        QString target = url;
        for (int depth = 0; depth < 2; ++depth) {
            auto pl = probe(client, target, headers, false, QStringLiteral("bytes=0-131071"));
            if (pl.code < 200 || pl.code >= 400) return false;
            if (!pl.body.startsWith("#EXTM3U")) return true;   // got media bytes - reachable

            const QUrl base(target);
            auto resolve = [&base](const QString &u) {
                return (QUrl(u).scheme().isEmpty() ? base.resolved(QUrl(u)) : QUrl(u)).toString();
            };

            // Encryption key (lives in the media playlist) must be fetchable.
            static QRegularExpression keyRe(QStringLiteral("#EXT-X-KEY:([^\r\n]+)"));
            auto keyMatch = keyRe.match(pl.body);
            if (keyMatch.hasMatch()) {
                static QRegularExpression uriRe(QString("URI=\"([^\"]+)\"|URI=([^\\s,]+)"));
                auto uriMatch = uriRe.match(keyMatch.captured(1));
                if (uriMatch.hasMatch()) {
                    const QString keyUri = uriMatch.captured(1).isEmpty() ? uriMatch.captured(2) : uriMatch.captured(1);
                    const QString keyUrl = resolve(keyUri);
                    auto keyResp = probe(client, keyUrl, headers, true);
                    if (keyResp.code < 200 || keyResp.code >= 400) {
                        auto keyGet = probe(client, keyUrl, headers, false);
                        if (keyGet.code < 200 || keyGet.code >= 400 || keyGet.body.isEmpty()) return false;
                    }
                }
            }

            const QStringList lines = pl.body.split('\n');

            // Master playlist -> descend into the first variant's media playlist.
            QString variant;
            for (int i = 0; i < lines.size(); ++i) {
                if (lines[i].trimmed().startsWith("#EXT-X-STREAM-INF")) {
                    for (int j = i + 1; j < lines.size(); ++j) {
                        const QString u = lines[j].trimmed();
                        if (!u.isEmpty() && !u.startsWith('#')) { variant = u; break; }
                    }
                    break;
                }
            }
            if (!variant.isEmpty()) { target = resolve(variant); continue; }

            // Media playlist -> probe the first real segment for bytes.
            QString segment;
            for (const auto &ln : lines) {
                const QString t = ln.trimmed();
                if (!t.isEmpty() && !t.startsWith('#')) { segment = t; break; }
            }
            if (segment.isEmpty()) return false;
            const QString segUrl = resolve(segment);
            auto segResp = probe(client, segUrl, headers, false, QStringLiteral("bytes=0-0"));
            if (segResp.code == 200 || segResp.code == 206) return true;
            auto segHead = probe(client, segUrl, headers, true);
            return segHead.code >= 200 && segHead.code < 400;
        }
        return false;   // nested masters beyond two levels - can't verify
    }

    auto ranged = probe(client, url, headers, false, QStringLiteral("bytes=0-0"));
    if (ranged.code == 200 || ranged.code == 206) return true;
    if (headResp.code >= 200 && headResp.code < 400) {
        bool isMp4 = url.endsWith(".mp4", Qt::CaseInsensitive) || contentType.startsWith("video/mp4");
        if (isMp4 || contentType.startsWith("video/") || contentType.isEmpty())
            return true;
    }
    return false;
}

ServerSelector::Result ServerSelector::findWorkingServer(Client *client, ShowProvider *provider, QList<VideoServer> &servers) {
    Result result;

    const auto want = Settings::instance().preferDub() ? VideoServer::Dub : VideoServer::Sub;
    const bool hasPreferredLang =
        std::any_of(servers.begin(), servers.end(),
                    [want](const VideoServer &s) { return s.translation == want; });

    // (A) The race finishes on whoever answers first, usually the lowest resolution,
    // so quality is picked here. Hosts that name no quality score 0.
    QString preferred = provider->getPreferredServer();
    const bool userChose = !preferred.isEmpty();
    if (!userChose) {
        int best = -1;
        for (int i = 0; i < servers.size(); ++i) {
            if (hasPreferredLang && servers[i].translation != want) continue;
            if (servers[i].resolution() == 0) continue;
            if (best < 0 || servers[i].resolution() > servers[best].resolution()) best = i;
        }
        if (best >= 0) preferred = servers[best].name;
    }

    if (!preferred.isEmpty()) {
        auto it = std::find_if(servers.begin(), servers.end(),
                               [&](const VideoServer &s) { return s.name == preferred; });
        if (it != servers.end() && (it->translation == want || !hasPreferredLang)) {
            int idx = std::distance(servers.begin(), it);
            // A preferred server that throws (Iyf key refresh) falls through to the race.
            const char *why = userChose ? "preferred server" : "best quality";
            try {
                auto playInfo = provider->extractSource(client, *it);
                if (checkVideo(client, playInfo)) {
                    gLog() << "Server" << "Using" << why << it->name;
                    result.cachedSources.insert(it->name, playInfo);
                    result.index = idx;
                    result.playInfo = std::move(playInfo);
                    return result;
                }
                oLog() << "Server" << why << it->name << "is broken";
            } catch (AppException &e) {
                e.print();
            } catch (const std::exception &e) {
                oLog() << "Server" << why << it->name << "failed:" << e.what();
            }
        }
    }

    // (B) Race preferred-language servers first, the rest only if none works.
    const auto rest = std::stable_partition(servers.begin(), servers.end(),
                                            [want](const VideoServer &s) { return s.translation == want; });
    int pivot = int(std::distance(servers.begin(), rest));
    if (pivot == 0) pivot = servers.size();  // no preferred-language servers - race all

    std::mutex resultMutex;
    QHash<QString, PlayInfo> extractedSources;
    int winner = -1;
    PlayInfo winnerPlayInfo;

    auto raceRange = [&](int lo, int hi) {
        if (lo >= hi || client->isCancelled()) return;
        std::atomic<int>  winnerIndex{-1};
        CancelToken       raceOver;

        QList<QFuture<bool>> jobs;
        jobs.reserve(hi - lo);
        for (int i = lo; i < hi; ++i) {
            if (client->isCancelled()) break;
            jobs.push_back(QtConcurrent::run([i, &servers, client, provider,
                                              &winnerIndex, &winnerPlayInfo,
                                              &resultMutex, &extractedSources,
                                              &raceOver]() -> bool {
                Client subClient = client->withCancel(raceOver);
                if (subClient.isCancelled()) return true;

                try {
                    auto playInfo = provider->extractSource(&subClient, servers[i]);
                    if (subClient.isCancelled()) return true;

                    if (checkVideo(&subClient, playInfo)) {
                        if (subClient.isCancelled()) return true;

                        std::lock_guard<std::mutex> lock(resultMutex);
                        extractedSources.insert(servers[i].name, playInfo);

                        int expected = -1;
                        if (winnerIndex.compare_exchange_strong(expected, i)) {
                            winnerPlayInfo = std::move(playInfo);
                            raceOver.cancel();
                            gLog() << "Server" << "Using" << servers[i].name;
                        }
                        return true;
                    }
                    if (subClient.isCancelled()) return true;   // lost the race, not broken
                    oLog() << "Server" << servers[i].name << "is broken";
                    return false;
                } catch (AppException &e) {
                    e.print();
                    return true;
                } catch (const std::exception &e) {
                    oLog() << "Server" << servers[i].name << e.what();
                    return true;
                } catch (...) {
                    oLog() << "Server" << servers[i].name << "unknown error";
                    return true;
                }
            }));
        }

        for (auto &job : jobs)
            job.waitForFinished();
        if (winnerIndex.load() >= 0) winner = winnerIndex.load();
    };

    raceRange(0, pivot);
    if (winner < 0)
        raceRange(pivot, servers.size());

    // No pruning: broken servers stay (marked Working/Broken later) so indices stay stable.
    result.index = winner;
    if (winner >= 0) {
        std::lock_guard<std::mutex> lock(resultMutex);
        result.playInfo = std::move(winnerPlayInfo);
    }
    result.cachedSources = std::move(extractedSources);
    return result;
}