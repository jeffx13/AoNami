#pragma once

#include <QObject>
#include <QSettings>
#include <QMap>
#include <QString>
#include "app/qml_singleton.h"

class Settings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool mpvLogEnabled READ mpvLogEnabled WRITE setMpvLogEnabled NOTIFY mpvLogEnabledChanged)
    Q_PROPERTY(bool mpvYtdlEnabled READ mpvYtdlEnabled WRITE setMpvYtdlEnabled NOTIFY mpvYtdlEnabledChanged)
    Q_PROPERTY(QString proxy READ proxy WRITE setProxy NOTIFY proxyChanged)

public:
    explicit Settings(QObject *parent = nullptr);
    static Settings& instance();

    // Generic helpers
    Q_INVOKABLE bool getBool(const QString &key, bool defaultValue = false) const;
    Q_INVOKABLE void setBool(const QString &key, bool value);
    Q_INVOKABLE QString getString(const QString &key, const QString &defaultValue = QString()) const;
    Q_INVOKABLE void setString(const QString &key, const QString &value);

    // MPV logging toggle
    bool mpvLogEnabled() const;
    void setMpvLogEnabled(bool enabled);

    // MPV ytdl/yt-dlp usage toggle
    bool mpvYtdlEnabled() const;
    void setMpvYtdlEnabled(bool enabled);

    // Proxy
    QString proxy() const;
    void setProxy(const QString &proxyString);
    
    
signals:
    void mpvLogEnabledChanged();
    void mpvYtdlEnabledChanged();
    void proxyChanged();

private:
    void applyProxySettings(const QString &proxyString);
    QSettings m_settings;

public:
    static QString getTempDir();
    // Read all key/value pairs under an INI group path like "bilibili/cookies"
    QMap<QString, QString> getGroupMap(const QString &group);
};

DECLARE_QML_SINGLETON(Settings);
