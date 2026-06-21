#pragma once
#include <QQmlNetworkAccessManagerFactory>
#include <QNetworkDiskCache>
#include <QStandardPaths>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QDir>

// NAM that injects a Referer for image hosts that 403 without one (AllAnime posters).
class RefererNAM : public QNetworkAccessManager {
public:
    using QNetworkAccessManager::QNetworkAccessManager;

protected:
    QNetworkReply *createRequest(Operation op, const QNetworkRequest &req, QIODevice *outgoingData) override {
        const QString host = req.url().host();
        if (host.endsWith("youtube-anime.com") && !req.hasRawHeader("Referer")) {
            QNetworkRequest r(req);
            r.setRawHeader("Referer", "https://youtu-chan.com/");
            return QNetworkAccessManager::createRequest(op, r, outgoingData);
        }
        return QNetworkAccessManager::createRequest(op, req, outgoingData);
    }
};

class ImageNAMFactory : public QQmlNetworkAccessManagerFactory {
public:
    QNetworkAccessManager *create(QObject *parent) override {
        constexpr qint64 k_maxCacheSize = 100LL * 1024 * 1024;
        QNetworkAccessManager *manager = new RefererNAM(parent);
        QNetworkDiskCache *diskCache = new QNetworkDiskCache(manager);
        QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/httpcache";
        QDir dir;
        if (!dir.exists(cacheDir)) dir.mkpath(cacheDir);
        diskCache->setCacheDirectory(cacheDir);
        diskCache->setMaximumCacheSize(k_maxCacheSize);
        manager->setCache(diskCache);
        return manager;
    }
    ~ImageNAMFactory() override = default;
};
