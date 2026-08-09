#include "settings.h"
#include "logger.h"
#include "core/danmaku.h"
#include "ui/uibridge.h"
#include <QNetworkProxyFactory>
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>

// Extraction runs off the GUI thread, where QSettings must not be touched.
static void syncDanmakuOptions(const Settings &s) {
    DanmakuOptions o;
    o.enabled      = s.danmakuEnabled();
    o.opacityPct   = s.danmakuOpacity();
    o.fontScalePct = s.danmakuFontScale();
    o.speedPct     = s.danmakuSpeed();
    o.areaPct      = s.danmakuArea();
    o.maxLines     = s.get(Config::DanmakuMaxLines);
    o.minWeight    = s.danmakuMinWeight();
    o.maxOnScreen  = s.danmakuMaxOnScreen();
    o.subScale     = s.subFontSize() / 40.0;
    o.font         = s.get(Config::DanmakuFont);
    o.bold         = s.danmakuBold();
    o.outline      = s.danmakuOutline();
    o.blockScroll  = s.danmakuBlockScroll();
    o.blockTop     = s.danmakuBlockTop();
    o.blockBottom  = s.danmakuBlockBottom();
    o.blockColour  = s.danmakuBlockColour();
    o.blockRepeat  = s.danmakuBlockRepeat();
    DanmakuOptions::set(o);
}

Settings::Settings(QObject *parent)
    : QObject(parent)
    , m_settings(QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("settings.ini"), QSettings::IniFormat)
{
    s_preferDub.store(get(Config::PreferDub), std::memory_order_relaxed);
    syncDanmakuOptions(*this);

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
    syncDanmakuOptions(*this);
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

void Settings::setAniskipAuto(bool v) {
    if (aniskipAuto() == v) return;
    set(Config::AniSkipAuto, v);
    emit aniskipAutoChanged();
}

void Settings::setWatchedPercent(int v) {
    v = qBound(0, v, 100);
    if (watchedPercent() == v) return;
    set(Config::WatchedPercent, v);
    emit watchedPercentChanged();
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

void Settings::setDanmakuEnabled(bool v) {
    if (danmakuEnabled() == v) return;
    set(Config::DanmakuEnabled, v);
    syncDanmakuOptions(*this);
    emit danmakuEnabledChanged();
}

void Settings::setDanmakuOpacity(int v) {
    v = qBound(10, v, 100);
    if (danmakuOpacity() == v) return;
    set(Config::DanmakuOpacity, v);
    syncDanmakuOptions(*this);
    emit danmakuOpacityChanged();
}

void Settings::setDanmakuFontScale(int v) {
    v = qBound(50, v, 200);
    if (danmakuFontScale() == v) return;
    set(Config::DanmakuFontScale, v);
    syncDanmakuOptions(*this);
    emit danmakuFontScaleChanged();
}

void Settings::setDanmakuSpeed(int v) {
    v = qBound(25, v, 400);
    if (danmakuSpeed() == v) return;
    set(Config::DanmakuSpeed, v);
    syncDanmakuOptions(*this);
    emit danmakuSpeedChanged();
}

void Settings::setDanmakuArea(int v) {
    v = qBound(10, v, 100);
    if (danmakuArea() == v) return;
    set(Config::DanmakuArea, v);
    syncDanmakuOptions(*this);
    emit danmakuAreaChanged();
}

void Settings::setDanmakuMinWeight(int v) {
    v = qBound(0, v, 11);
    if (danmakuMinWeight() == v) return;
    set(Config::DanmakuMinWeight, v);
    syncDanmakuOptions(*this);
    emit danmakuMinWeightChanged();
}

void Settings::setDanmakuMaxOnScreen(int v) {
    v = qBound(0, v, 500);
    if (danmakuMaxOnScreen() == v) return;
    set(Config::DanmakuMaxOnScreen, v);
    syncDanmakuOptions(*this);
    emit danmakuMaxOnScreenChanged();
}

void Settings::setDanmakuBold(bool v) {
    if (danmakuBold() == v) return;
    set(Config::DanmakuBold, v);
    syncDanmakuOptions(*this);
    emit danmakuBoldChanged();
}

void Settings::setDanmakuOutline(int v) {
    v = qBound(0, v, 2);
    if (danmakuOutline() == v) return;
    set(Config::DanmakuOutline, v);
    syncDanmakuOptions(*this);
    emit danmakuOutlineChanged();
}

void Settings::setDanmakuBlockScroll(bool v) {
    if (danmakuBlockScroll() == v) return;
    set(Config::DanmakuBlockScroll, v);
    syncDanmakuOptions(*this);
    emit danmakuBlockScrollChanged();
}

void Settings::setDanmakuBlockTop(bool v) {
    if (danmakuBlockTop() == v) return;
    set(Config::DanmakuBlockTop, v);
    syncDanmakuOptions(*this);
    emit danmakuBlockTopChanged();
}

void Settings::setDanmakuBlockBottom(bool v) {
    if (danmakuBlockBottom() == v) return;
    set(Config::DanmakuBlockBottom, v);
    syncDanmakuOptions(*this);
    emit danmakuBlockBottomChanged();
}

void Settings::setDanmakuBlockColour(bool v) {
    if (danmakuBlockColour() == v) return;
    set(Config::DanmakuBlockColour, v);
    syncDanmakuOptions(*this);
    emit danmakuBlockColourChanged();
}

void Settings::setDanmakuBlockRepeat(bool v) {
    if (danmakuBlockRepeat() == v) return;
    set(Config::DanmakuBlockRepeat, v);
    syncDanmakuOptions(*this);
    emit danmakuBlockRepeatChanged();
}

void Settings::setDiscordEnabled(bool v) {
    if (discordEnabled() == v) return;
    set(Config::DiscordEnabled, v);
    emit discordEnabledChanged();
}

void Settings::setThemeName(const QString &v) {
    if (themeName() == v) return;
    set(Config::ThemeName, v);
    emit themeNameChanged();
}

void Settings::setAccentColor(const QString &v) {
    if (accentColor() == v) return;
    set(Config::AccentColor, v);
    emit accentColorChanged();
}

void Settings::setUiScale(double v) {
    v = qBound(0.8, v, 1.4);
    if (qFuzzyCompare(uiScale(), v)) return;
    set(Config::UiScale, v);
    emit uiScaleChanged();
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
