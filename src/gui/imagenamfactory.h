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
        QNetworkDiskCache *diskCache = new QNetworkDiskCache(manager);
        QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/httpcache";
        QDir dir;
        if (!dir.exists(cacheDir)) dir.mkpath(cacheDir);
        diskCache->setCacheDirectory(cacheDir);
        diskCache->setMaximumCacheSize(100 * 1024 * 1024); // 100 MB
        manager->setCache(diskCache);
        return manager;
    }
    ~ImageNAMFactory() override = default;
};
