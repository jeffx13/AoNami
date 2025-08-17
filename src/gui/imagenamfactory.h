#pragma once
#include <QQmlNetworkAccessManagerFactory>
#include <QNetworkDiskCache>
#include <QStandardPaths>
#include <QNetworkAccessManager>
#include <QDir>
#include <QNetworkReply>

class ImageNAMFactory : public QQmlNetworkAccessManagerFactory {
public:
    QNetworkAccessManager *create(QObject *parent) override {
        QNetworkAccessManager *manager = new QNetworkAccessManager(parent);

        // Set up disk cache
        QNetworkDiskCache *diskCache = new QNetworkDiskCache(manager);

        // Create cache directory if it doesn't exist
        QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/httpcache";
        // qDebug() << "Using cache directory:" << cacheDir;
        QDir dir;
        if (!dir.exists(cacheDir)) {
            dir.mkpath(cacheDir);
            qDebug() << "Created cache directory:" << cacheDir;
        }

        diskCache->setCacheDirectory(cacheDir);
        diskCache->setMaximumCacheSize(100 * 1024 * 1024); // 100 MB

        // Optional: Set cache behavior
        // diskCache->setCacheSize(50 * 1024 * 1024); // Current size limit

        manager->setCache(diskCache);

        // Optional: Add some debugging
        // qDebug() << "Network cache initialized at:" << cacheDir;
        // qDebug() << "Cache max size:" << diskCache->maximumCacheSize() / (1024 * 1024) << "MB";

        // Optional: Configure for better image caching
        // manager->setNetworkAccessible(QNetworkAccessManager::Accessible);

        // Optional: Handle cache statistics (for debugging)
        // QObject::connect(manager, &QNetworkAccessManager::finished, [diskCache](QNetworkReply *reply) {
        //     if (reply->error() == QNetworkReply::NoError) {
        //         // Check if response was cached
        //         bool fromCache = reply->attribute(QNetworkRequest::SourceIsFromCacheAttribute).toBool();
        //         if (fromCache) {
        //             qDebug() << "Cache HIT for:" << reply->url().toString();
        //         } else {
        //             qDebug() << "Cache MISS for:" << reply->url().toString();
        //         }
        //     }
        // });
        return manager;
    }
    ~ImageNAMFactory() override = default;
};
