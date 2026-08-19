#pragma once
#include <QQmlNetworkAccessManagerFactory>
#include <QNetworkDiskCache>
#include <QStandardPaths>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QDir>
#include "net/cloudflare.h"

// A separate stack from Client's, so the clearance and its UA have to be applied here too -
// otherwise posters 403 while the API behind them works fine.
class RefererNam : public QNetworkAccessManager {
public:
    using QNetworkAccessManager::QNetworkAccessManager;

protected:
    QNetworkReply *createRequest(Operation op, const QNetworkRequest &req, QIODevice *outgoingData) override {
        const QString host = req.url().host();
        QNetworkRequest r(req);
        bool modified = false;

        // AllAnime posters 403 without one.
        if (host.endsWith("youtube-anime.com") && !req.hasRawHeader("Referer")) {
            r.setRawHeader("Referer", "https://youtu-chan.com/");
            modified = true;
        }

        if (const QString hostUa = Cloudflare::hostUserAgent(host); !hostUa.isEmpty()) {
            r.setRawHeader("User-Agent", hostUa.toUtf8());
            modified = true;
        }

        return QNetworkAccessManager::createRequest(op, modified ? r : req, outgoingData);
    }
};

class ImageFactory : public QQmlNetworkAccessManagerFactory {
public:
    QNetworkAccessManager *create(QObject *parent) override {
        constexpr qint64 k_maxCacheSize = 100LL * 1024 * 1024;
        QNetworkAccessManager *manager = new RefererNam(parent);
        manager->setCookieJar(new Cloudflare::ProxyCookieJar(manager));
        QNetworkDiskCache *diskCache = new QNetworkDiskCache(manager);
        QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/httpcache";
        QDir dir;
        if (!dir.exists(cacheDir)) dir.mkpath(cacheDir);
        diskCache->setCacheDirectory(cacheDir);
        diskCache->setMaximumCacheSize(k_maxCacheSize);
        manager->setCache(diskCache);
        return manager;
    }
    ~ImageFactory() override = default;
};
