#include "serverselector.h"
#include "providers/showprovider.h"
#include "app/logger.h"
#include "app/settings.h"
#include <QtConcurrent/QtConcurrentRun>
#include <atomic>
#include <mutex>
#include <algorithm>
#include <QUrl>
#include <QRegularExpression>

bool ServerSelector::checkVideo(Client *client, PlayInfo &playItem) {
    if (playItem.videos.isEmpty()) return false;
    const auto &video = playItem.videos.first();
    if (video.url.isLocalFile()) return true;

    const QString url = video.url.toString();
    const auto &headers = playItem.headers;

    auto headResp = client->head(url, headers);
    // No response (timeout/connection failure) -> host unreachable; bail before retrying.
    if (headResp.code <= 0) return false;
    const QString contentType = headResp.header("Content-Type").toLower();

    bool isHls = url.endsWith(".m3u8", Qt::CaseInsensitive)
                 || contentType.contains("application/vnd.apple.mpegurl")
                 || contentType.contains("application/x-mpegurl");

    if (isHls) {
        // Probe down to a real segment - intact playlists with 404 segments fake "working".
        QString target = url;
        for (int depth = 0; depth < 2; ++depth) {
            auto plHeaders = headers;
            plHeaders.insert("Range", "bytes=0-131071");
            auto pl = client->get(target, plHeaders);
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
                    auto keyResp = client->head(keyUrl, headers);
                    if (keyResp.code < 200 || keyResp.code >= 400) {
                        auto keyGet = client->get(keyUrl, headers);
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
            if (client->partialGet(segUrl, headers)) return true;
            auto segHead = client->head(segUrl, headers);
            return segHead.code >= 200 && segHead.code < 400;
        }
        return false;   // nested masters beyond two levels - can't verify
    }

    if (client->partialGet(url, headers)) return true;
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

    // (A) Reuse the last working server if its language still matches the preference.
    QString preferred = provider->getPreferredServer();
    if (!preferred.isEmpty()) {
        auto it = std::find_if(servers.begin(), servers.end(),
                               [&](const VideoServer &s) { return s.name == preferred; });
        if (it != servers.end() && (it->translation == want || !hasPreferredLang)) {
            int idx = std::distance(servers.begin(), it);
            auto playInfo = provider->extractSource(client, *it);
            if (checkVideo(client, playInfo)) {
                gLog() << "Server" << "Using preferred server" << it->name;
                result.cachedSources.insert(it->name, playInfo);
                result.index = idx;
                result.playInfo = std::move(playInfo);
                return result;
            }
            oLog() << "Server" << "Preferred server" << it->name << "is broken";
            // Keep it in the list; fall through to race the others.
        }
    }

    // (B) Race preferred-language servers first, the rest only if none works.
    std::stable_partition(servers.begin(), servers.end(),
                          [want](const VideoServer &s) { return s.translation == want; });
    int pivot = 0;
    while (pivot < servers.size() && servers[pivot].translation == want)
        ++pivot;
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

                        // Cache this extraction regardless of whether we win the race
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