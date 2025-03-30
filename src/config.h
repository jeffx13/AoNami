#pragma once
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

    static bool load() {
        QString appDir = QCoreApplication::applicationDirPath();
        QString configPath = appDir + QDir::separator() + "config";

        // qDebug() << "Looking for config at:" << configPath;

        // Step 2: Check if file exists
        QFile configFile(configPath);
        if (!configFile.exists()) {
            qDebug() << "Config file not found.";
            return false;
        }

        // Step 3: Open and read the file
        if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "Failed to open config file.";
            return false;
        }

        QByteArray configFileData = configFile.readAll();
        configFile.close();

        // Step 4: Parse JSON
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(configFileData, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qDebug() << "JSON parse error:" << parseError.errorString();
            return false;
        }

        if (!doc.isObject()) {
            qDebug() << "Config file is not a valid JSON object.";
            return false;
        }

        data = doc.object();
        return true;
    }
};
