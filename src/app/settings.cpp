#include "app/settings.h"
#include "media/danmaku.h"
#include "ui/uibridge.h"
#include <QNetworkProxyFactory>
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>

void Settings::syncDanmakuOptions() const {
    DanmakuOptions o;
    o.enabled      = danmakuEnabled();
    o.opacityPct   = danmakuOpacity();
    o.fontScalePct = danmakuFontScale();
    o.speedPct     = danmakuSpeed();
    o.areaPct      = danmakuArea();
    o.maxLines     = get(Config::DanmakuMaxLines);
    o.minWeight    = danmakuMinWeight();
    o.maxOnScreen  = danmakuMaxOnScreen();

    o.font         = get(Config::DanmakuFont);
    o.bold         = danmakuBold();
    o.outline      = danmakuOutline();
    o.blockScroll  = danmakuBlockScroll();
    o.blockTop     = danmakuBlockTop();
    o.blockBottom  = danmakuBlockBottom();
    o.blockColour  = danmakuBlockColour();
    o.blockRepeat  = danmakuBlockRepeat();
    DanmakuOptions::set(o);
}

Settings::Settings(QObject *parent)
    : QObject(parent)
    , m_settings(QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("settings.ini"), QSettings::IniFormat)
{
    s_preferDub.store(get(Config::PreferDub), std::memory_order_relaxed);
    syncDanmakuOptions();

    if (const QString savedProxy = proxy(); !savedProxy.isEmpty())
        applyProxySettings(savedProxy);

    m_syncTimer.setSingleShot(true);
    m_syncTimer.setInterval(400);
    connect(&m_syncTimer, &QTimer::timeout, this, [this]() { m_settings.sync(); });

    m_fileWatcher.addPath(getPath());
    connect(&m_fileWatcher, &QFileSystemWatcher::fileChanged, this, [this](const QString &path) {
        if (path == getPath()) {
            m_settings.sync();
            // Only the setters refresh these, so a hand-edited file would leave them stale.
            s_preferDub.store(get(Config::PreferDub), std::memory_order_relaxed);
            syncDanmakuOptions();
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

QString Settings::appDir() const {
    return QCoreApplication::applicationDirPath();
}

void Settings::scheduleSync() {
    m_syncTimer.start();
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

// download/dir has a runtime default and is validated, so it stays off the Config::Key path.
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
    emit settingsChanged();
}

void Settings::setProxy(const QString &v) {
    if (apply(Config::Proxy, v, &Settings::proxyChanged))
        applyProxySettings(v);
}

void Settings::setPreferDub(bool v) {
    if (apply(Config::PreferDub, v, &Settings::preferDubChanged))
        s_preferDub.store(v, std::memory_order_relaxed);
}

void Settings::setUiScale(double v) {
    v = qBound(0.8, v, 1.4);
    if (qFuzzyCompare(uiScale(), v)) return;
    set(Config::UiScale, v);
    emit uiScaleChanged();
    emit settingsChanged();
}

void Settings::resetDanmakuAppearance() {
    setDanmakuOpacity(Config::DanmakuOpacity.defaultValue);
    setDanmakuFontScale(Config::DanmakuFontScale.defaultValue);
    setDanmakuSpeed(Config::DanmakuSpeed.defaultValue);
    setDanmakuArea(Config::DanmakuArea.defaultValue);
    setDanmakuMinWeight(Config::DanmakuMinWeight.defaultValue);
    setDanmakuMaxOnScreen(Config::DanmakuMaxOnScreen.defaultValue);
    setDanmakuBold(Config::DanmakuBold.defaultValue);
    setDanmakuOutline(Config::DanmakuOutline.defaultValue);
    setDanmakuBlockScroll(Config::DanmakuBlockScroll.defaultValue);
    setDanmakuBlockTop(Config::DanmakuBlockTop.defaultValue);
    setDanmakuBlockBottom(Config::DanmakuBlockBottom.defaultValue);
    setDanmakuBlockColour(Config::DanmakuBlockColour.defaultValue);
    setDanmakuBlockRepeat(Config::DanmakuBlockRepeat.defaultValue);
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
