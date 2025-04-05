#pragma once
#include "utils/logger.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>


class Config
{
    inline static QJsonObject data{};
public:

    static QJsonObject& get() {
        return data;
    }
    static QString getTempDir() {
        QString appDir = QCoreApplication::applicationDirPath();
        QString tempDirPath = appDir + QDir::separator() + ".tmp";
        QDir tempDir(tempDirPath);
        if (!tempDir.exists()) {
            tempDir.mkpath(tempDirPath);
        }
        return tempDirPath;
    }

    static bool load() {
        // delete files in temp dir
        QString tempDir = getTempDir();
        QDir tempDirPath(tempDir);
        if (tempDirPath.exists()) {
            for (const QString& file : tempDirPath.entryList(QDir::Files)) {
                QFile::remove(tempDirPath.absoluteFilePath(file));
            }
        }

        QString appDir = QCoreApplication::applicationDirPath();
        QString configPath = appDir + QDir::separator() + ".config";


        QFile configFile(configPath);
        if (!configFile.exists()) {
            rLog() << "Config" << "Config file not found.";
            return false;
        }

        if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            rLog() << "Config" << "Failed to open config file.";
            return false;
        }

        QByteArray configFileData = configFile.readAll();
        configFile.close();

        // Step 4: Parse JSON
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(configFileData, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            rLog() << "Config" << "JSON parse error:" << parseError.errorString();
            return false;
        }

        if (!doc.isObject()) {
            rLog() << "Config" << "Config file is not a valid JSON object.";
            return false;
        }

        data = doc.object();
        return true;
    }
};
