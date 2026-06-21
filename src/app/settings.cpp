#include "settings.h"
#include "logger.h"
#include "ui/uibridge.h"
#include <QNetworkProxyFactory>
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>

Settings::Settings(QObject *parent)
    : QObject(parent)
    , m_settings(QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("settings.ini"), QSettings::IniFormat)
{
    s_preferDub.store(get(Config::PreferDub), std::memory_order_relaxed);

    QString savedProxy = proxy();
    if (!savedProxy.isEmpty())
        applyProxySettings(savedProxy);

    m_syncTimer.setSingleShot(true);
    m_syncTimer.setInterval(400);
    connect(&m_syncTimer, &QTimer::timeout, this, [this]() { m_settings.sync(); });

    // Watch for external edits
    m_fileWatcher.addPath(getPath());
    connect(&m_fileWatcher, &QFileSystemWatcher::fileChanged, this, [this](const QString &path) {
        if (path == getPath()) {
            m_settings.sync();
            emit settingsChanged();
        }
        // Editors that replace-on-save drop the watch; re-add it so future edits fire.
        if (!m_fileWatcher.files().contains(path) && QFileInfo::exists(path))
            m_fileWatcher.addPath(path);
    });
}

Settings &Settings::instance() {
    static Settings s_instance;
    return s_instance;
}

QString Settings::getPath() const {
    return QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("settings.ini");
}

void Settings::scheduleSync() {
    m_syncTimer.start();
}

QString Settings::appDir() const {
    return QCoreApplication::applicationDirPath();
}

bool Settings::getBool(const QString &key, bool defaultValue) const {
    return m_settings.value(key, defaultValue).toBool();
}

void Settings::setBool(const QString &key, bool value) {
    m_settings.setValue(key, value);
    scheduleSync();
}

QString Settings::getString(const QString &key, const QString &defaultValue) const {
    return m_settings.value(key, defaultValue).toString();
}

void Settings::setString(const QString &key, const QString &value) {
    m_settings.setValue(key, value);
    scheduleSync();
}

QStringList Settings::getStringList(const QString &key) const {
    return m_settings.value(key).toStringList();
}

void Settings::prependToHistory(const QString &key, const QString &value, int maxCount) {
    if (value.trimmed().isEmpty()) return;
    QStringList list = getStringList(key);
    list.removeAll(value);
    list.prepend(value);
    while (list.size() > maxCount)
        list.removeLast();
    m_settings.setValue(key, list);
    m_settings.sync();
}

void Settings::removeFromHistory(const QString &key, const QString &value) {
    QStringList list = getStringList(key);
    list.removeAll(value);
    m_settings.setValue(key, list);
    m_settings.sync();
}

void Settings::clearHistory(const QString &key) {
    m_settings.remove(key);
    m_settings.sync();
}

void Settings::setMpvLogEnabled(bool enabled) {
    if (mpvLogEnabled() == enabled) return;
    set(Config::MpvLog, enabled);
    emit mpvLogEnabledChanged();
}

void Settings::setMpvYtdlEnabled(bool enabled) {
    if (mpvYtdlEnabled() == enabled) return;
    set(Config::Ytdl, enabled);
    emit mpvYtdlEnabledChanged();
}

QString Settings::downloadDir() const {
    return m_settings.value("download/dir",
                            QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)).toString();
}

void Settings::setDownloadDir(const QString &dir) {
    if (downloadDir() == dir) return;
    QFileInfo outputDir(dir);
    if (!outputDir.exists() || !outputDir.isDir() || !outputDir.isWritable()) {
        UiBridge::instance().showError(QString("Invalid output directory: %1").arg(outputDir.absoluteFilePath()));
        return;
    }
    m_settings.setValue("download/dir", dir);
    scheduleSync();
    emit downloadDirChanged();
}

void Settings::setProxy(const QString &proxyString) {
    if (proxy() == proxyString) return;
    set(Config::Proxy, proxyString);
    applyProxySettings(proxyString);
    emit proxyChanged();
}

void Settings::setMaxSpeed(const QString &v) {
    if (maxSpeed() == v) return;
    set(Config::MaxSpeed, v);
    emit maxSpeedChanged();
}

void Settings::setSubFontSize(int v) {
    if (subFontSize() == v) return;
    set(Config::SubFontSize, v);
    emit subFontSizeChanged();
}

void Settings::setSubPos(int v) {
    if (subPos() == v) return;
    set(Config::SubPos, v);
    emit subPosChanged();
}

void Settings::setPreferDub(bool v) {
    if (preferDub() == v) return;
    s_preferDub.store(v, std::memory_order_relaxed);
    set(Config::PreferDub, v);
    emit preferDubChanged();
}

void Settings::setAniskipEnabled(bool v) {
    if (aniskipEnabled() == v) return;
    set(Config::AniSkip, v);
    emit aniskipEnabledChanged();
}

void Settings::setWatchedPercent(int v) {
    v = qBound(0, v, 100);
    if (watchedPercent() == v) return;
    set(Config::WatchedPercent, v);
    emit watchedPercentChanged();
}

void Settings::setDiscordEnabled(bool v) {
    if (discordEnabled() == v) return;
    set(Config::DiscordEnabled, v);
    emit discordEnabledChanged();
}

void Settings::applyProxySettings(const QString &proxyString) {
    QByteArray proxyBytes = proxyString.toUtf8();
    qputenv("http_proxy", proxyBytes);
    qputenv("https_proxy", proxyBytes);
    QNetworkProxyFactory::setUseSystemConfiguration(proxyString.isEmpty());
}

QString Settings::getTempDir() {
    QString path = QCoreApplication::applicationDirPath() + QDir::separator() + ".tmp";
    QDir dir(path);
    if (!dir.exists()) dir.mkpath(path);
    return path;
}

QMap<QString, QString> Settings::getGroupMap(const QString &group) const {
    QMap<QString, QString> map;
    m_settings.beginGroup(group);
    const auto keys = m_settings.childKeys();
    for (const auto &k : keys)
        map.insert(k, m_settings.value(k).toString());
    m_settings.endGroup();
    return map;
}
