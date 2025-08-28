#include "settings.h"
#include "logger.h"
#include <QNetworkProxyFactory>
#include <QCoreApplication>
#include <QDir>
#include <QByteArray>
#include <QStringList>
#include <QStandardPaths>

Settings::Settings(QObject *parent)
    : QObject{parent}
    , m_settings{QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("settings.ini"), QSettings::IniFormat}
{
    QString savedProxy = proxy();
    if (!savedProxy.isEmpty()) {
        applyProxySettings(savedProxy);
    }
    if (m_settings.value(QStringLiteral("logging/mpv")).isNull())
        m_settings.setValue(QStringLiteral("logging/mpv"), true);
    if (m_settings.value(QStringLiteral("player/ytdl")).isNull())
        m_settings.setValue(QStringLiteral("player/ytdl"), false);
    if (m_settings.value(QStringLiteral("network/proxy")).isNull())
        m_settings.setValue(QStringLiteral("network/proxy"), QString());
    m_settings.sync();
}

Settings &Settings::instance()
{
    static Settings s_instance;
    return s_instance;
}

bool Settings::getBool(const QString &key, bool defaultValue) const
{
    return m_settings.value(key, defaultValue).toBool();
}

void Settings::setBool(const QString &key, bool value)
{
    m_settings.setValue(key, value);
    m_settings.sync();
}

QString Settings::getString(const QString &key, const QString &defaultValue) const
{
    return m_settings.value(key, defaultValue).toString();
}

void Settings::setString(const QString &key, const QString &value)
{
    m_settings.setValue(key, value);
    m_settings.sync();
}

bool Settings::mpvLogEnabled() const
{
    return m_settings.value(QStringLiteral("logging/mpv"), true).toBool();
}

void Settings::setMpvLogEnabled(bool enabled)
{
    if (mpvLogEnabled() == enabled) return;
    m_settings.setValue(QStringLiteral("logging/mpv"), enabled);
    m_settings.sync();
    emit mpvLogEnabledChanged();
}

bool Settings::mpvYtdlEnabled() const
{
    return m_settings.value(QStringLiteral("player/ytdl"), false).toBool();
}

void Settings::setMpvYtdlEnabled(bool enabled)
{
    if (mpvYtdlEnabled() == enabled) return;
    m_settings.setValue(QStringLiteral("player/ytdl"), enabled);
    m_settings.sync();
    emit mpvYtdlEnabledChanged();
}

QString Settings::downloadDir() const
{
    return m_settings.value(QStringLiteral("download/dir"), QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)).toString();
}

void Settings::setDownloadDir(const QString &dir)
{
    if (downloadDir() == dir) return;
    QFileInfo outputDir(dir);
    if (!outputDir.exists() || !outputDir.isDir() || !outputDir.isWritable()) {
        oLog() << "Settings" << "Output directory either doesn't exist or isn't a directory or writeable"
               << outputDir.absoluteFilePath();
        return;
    }
    m_settings.setValue(QStringLiteral("download/dir"), dir);
    m_settings.sync();
    emit downloadDirChanged();
}

QString Settings::proxy() const
{
    return m_settings.value(QStringLiteral("network/proxy"), QString()).toString();
}

void Settings::setProxy(const QString &proxyString)
{
    if (proxy() == proxyString) return;
    m_settings.setValue(QStringLiteral("network/proxy"), proxyString);
    m_settings.sync();
    applyProxySettings(proxyString);
    emit proxyChanged();
}

void Settings::applyProxySettings(const QString &proxyString)
{
    QByteArray proxy = proxyString.toUtf8();
    qputenv("http_proxy", proxy);
    qputenv("https_proxy", proxy);
    QNetworkProxyFactory::setUseSystemConfiguration(proxyString.isEmpty());
}

QString Settings::getTempDir()
{
    QString appDir = QCoreApplication::applicationDirPath();
    QString tempDirPath = appDir + QDir::separator() + ".tmp";
    QDir tempDir(tempDirPath);
    if (!tempDir.exists()) tempDir.mkpath(tempDirPath);
    return tempDirPath;
}

QMap<QString, QString> Settings::getGroupMap(const QString &group)
{
    QMap<QString, QString> map;
    m_settings.beginGroup(group);
    const auto keys = m_settings.childKeys();
    for (const auto &k : keys) map.insert(k, m_settings.value(k).toString());
    m_settings.endGroup();
    return map;
}
