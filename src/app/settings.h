#pragma once
#include <QObject>
#include <QSettings>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QVariant>
#include <QMap>
#include <QString>
#include <atomic>
#include "app/qmlsingleton.h"
#include "app/config.h"

class Settings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool    mpvLogEnabled   READ mpvLogEnabled   WRITE setMpvLogEnabled   NOTIFY mpvLogEnabledChanged)
    Q_PROPERTY(bool    mpvYtdlEnabled  READ mpvYtdlEnabled  WRITE setMpvYtdlEnabled  NOTIFY mpvYtdlEnabledChanged)
    Q_PROPERTY(QString proxy           READ proxy           WRITE setProxy           NOTIFY proxyChanged)
    Q_PROPERTY(QString downloadDir     READ downloadDir     WRITE setDownloadDir     NOTIFY downloadDirChanged)
    Q_PROPERTY(QString maxSpeed        READ maxSpeed        WRITE setMaxSpeed        NOTIFY maxSpeedChanged)
    Q_PROPERTY(int     subFontSize     READ subFontSize     WRITE setSubFontSize     NOTIFY subFontSizeChanged)
    Q_PROPERTY(int     subPos          READ subPos          WRITE setSubPos          NOTIFY subPosChanged)
    Q_PROPERTY(bool    preferDub       READ preferDub       WRITE setPreferDub       NOTIFY preferDubChanged)
    Q_PROPERTY(bool    aniskipEnabled  READ aniskipEnabled  WRITE setAniskipEnabled  NOTIFY aniskipEnabledChanged)
    Q_PROPERTY(bool    aniskipAuto     READ aniskipAuto     WRITE setAniskipAuto     NOTIFY aniskipAutoChanged)
    Q_PROPERTY(int     watchedPercent  READ watchedPercent  WRITE setWatchedPercent  NOTIFY watchedPercentChanged)
    Q_PROPERTY(bool    danmakuEnabled     READ danmakuEnabled     WRITE setDanmakuEnabled     NOTIFY danmakuEnabledChanged)
    Q_PROPERTY(int     danmakuOpacity     READ danmakuOpacity     WRITE setDanmakuOpacity     NOTIFY danmakuOpacityChanged)
    Q_PROPERTY(int     danmakuFontScale   READ danmakuFontScale   WRITE setDanmakuFontScale   NOTIFY danmakuFontScaleChanged)
    Q_PROPERTY(int     danmakuSpeed       READ danmakuSpeed       WRITE setDanmakuSpeed       NOTIFY danmakuSpeedChanged)
    Q_PROPERTY(int     danmakuArea        READ danmakuArea        WRITE setDanmakuArea        NOTIFY danmakuAreaChanged)
    Q_PROPERTY(int     danmakuMinWeight   READ danmakuMinWeight   WRITE setDanmakuMinWeight   NOTIFY danmakuMinWeightChanged)
    Q_PROPERTY(int     danmakuMaxOnScreen READ danmakuMaxOnScreen WRITE setDanmakuMaxOnScreen NOTIFY danmakuMaxOnScreenChanged)
    Q_PROPERTY(bool    danmakuBold        READ danmakuBold        WRITE setDanmakuBold        NOTIFY danmakuBoldChanged)
    Q_PROPERTY(int     danmakuOutline     READ danmakuOutline     WRITE setDanmakuOutline     NOTIFY danmakuOutlineChanged)
    Q_PROPERTY(bool    danmakuBlockScroll READ danmakuBlockScroll WRITE setDanmakuBlockScroll NOTIFY danmakuBlockScrollChanged)
    Q_PROPERTY(bool    danmakuBlockTop    READ danmakuBlockTop    WRITE setDanmakuBlockTop    NOTIFY danmakuBlockTopChanged)
    Q_PROPERTY(bool    danmakuBlockBottom READ danmakuBlockBottom WRITE setDanmakuBlockBottom NOTIFY danmakuBlockBottomChanged)
    Q_PROPERTY(bool    danmakuBlockColour READ danmakuBlockColour WRITE setDanmakuBlockColour NOTIFY danmakuBlockColourChanged)
    Q_PROPERTY(bool    danmakuBlockRepeat READ danmakuBlockRepeat WRITE setDanmakuBlockRepeat NOTIFY danmakuBlockRepeatChanged)
    Q_PROPERTY(bool    discordEnabled  READ discordEnabled  WRITE setDiscordEnabled  NOTIFY discordEnabledChanged)
    Q_PROPERTY(QString themeName       READ themeName       WRITE setThemeName       NOTIFY themeNameChanged)
    Q_PROPERTY(QString accentColor     READ accentColor     WRITE setAccentColor     NOTIFY accentColorChanged)
    Q_PROPERTY(double  uiScale         READ uiScale         WRITE setUiScale         NOTIFY uiScaleChanged)
    Q_PROPERTY(QString path            READ getPath         CONSTANT)
    Q_PROPERTY(QString appDir          READ appDir          CONSTANT)

public:
    explicit Settings(QObject *parent = nullptr);
    static Settings& instance();

    template <typename T>
    T get(const Config::Key<T> &key) const {
        return m_settings.value(QLatin1String(key.path), QVariant::fromValue(key.defaultValue)).template value<T>();
    }
    template <typename T>
    void set(const Config::Key<T> &key, const T &value) {
        m_settings.setValue(QLatin1String(key.path), QVariant::fromValue(value));
        scheduleSync();
    }

    QString getPath() const;
    QString appDir() const;

    Q_INVOKABLE bool getBool(const QString &key, bool defaultValue = false) const;
    Q_INVOKABLE void setBool(const QString &key, bool value);
    Q_INVOKABLE QString getString(const QString &key, const QString &defaultValue = QString()) const;
    Q_INVOKABLE void setString(const QString &key, const QString &value);
    Q_INVOKABLE QStringList getStringList(const QString &key) const;
    Q_INVOKABLE void prependToHistory(const QString &key, const QString &value, int maxCount = 10);
    Q_INVOKABLE void removeFromHistory(const QString &key, const QString &value);
    Q_INVOKABLE void clearHistory(const QString &key);

    bool mpvLogEnabled() const  { return get(Config::MpvLog); }
    void setMpvLogEnabled(bool enabled);

    bool mpvYtdlEnabled() const { return get(Config::Ytdl); }
    void setMpvYtdlEnabled(bool enabled);

    QString downloadDir() const;
    void setDownloadDir(const QString &dir);

    QString proxy() const       { return get(Config::Proxy); }
    void setProxy(const QString &proxyString);

    QString maxSpeed() const    { return get(Config::MaxSpeed); }
    void setMaxSpeed(const QString &v);

    int subFontSize() const     { return get(Config::SubFontSize); }
    void setSubFontSize(int v);

    int subPos() const          { return get(Config::SubPos); }
    void setSubPos(int v);

    // Cached in an atomic so the server selector worker can read it off the main thread.
    bool preferDub() const      { return s_preferDub.load(std::memory_order_relaxed); }
    void setPreferDub(bool v);

    bool aniskipEnabled() const { return get(Config::AniSkip); }
    void setAniskipEnabled(bool v);
    bool aniskipAuto() const { return get(Config::AniSkipAuto); }
    void setAniskipAuto(bool v);

    int watchedPercent() const  { return get(Config::WatchedPercent); }
    void setWatchedPercent(int v);

    // Progress at or above this counts the episode as watched.
    double watchedFraction() const { return qBound(1, watchedPercent(), 100) / 100.0; }

    // Appearance only, not the enable switch.
    Q_INVOKABLE void resetDanmakuAppearance();

    bool danmakuEnabled() const     { return get(Config::DanmakuEnabled); }
    void setDanmakuEnabled(bool v);
    int danmakuOpacity() const      { return get(Config::DanmakuOpacity); }
    void setDanmakuOpacity(int v);
    int danmakuFontScale() const    { return get(Config::DanmakuFontScale); }
    void setDanmakuFontScale(int v);
    int danmakuSpeed() const        { return get(Config::DanmakuSpeed); }
    void setDanmakuSpeed(int v);
    int danmakuArea() const         { return get(Config::DanmakuArea); }
    void setDanmakuArea(int v);
    int danmakuMinWeight() const    { return get(Config::DanmakuMinWeight); }
    void setDanmakuMinWeight(int v);
    int danmakuMaxOnScreen() const  { return get(Config::DanmakuMaxOnScreen); }
    void setDanmakuMaxOnScreen(int v);
    bool danmakuBold() const { return get(Config::DanmakuBold); }
    void setDanmakuBold(bool v);
    int danmakuOutline() const { return get(Config::DanmakuOutline); }
    void setDanmakuOutline(int v);
    bool danmakuBlockScroll() const { return get(Config::DanmakuBlockScroll); }
    void setDanmakuBlockScroll(bool v);
    bool danmakuBlockTop() const { return get(Config::DanmakuBlockTop); }
    void setDanmakuBlockTop(bool v);
    bool danmakuBlockBottom() const { return get(Config::DanmakuBlockBottom); }
    void setDanmakuBlockBottom(bool v);
    bool danmakuBlockColour() const { return get(Config::DanmakuBlockColour); }
    void setDanmakuBlockColour(bool v);
    bool danmakuBlockRepeat() const { return get(Config::DanmakuBlockRepeat); }
    void setDanmakuBlockRepeat(bool v);

    bool discordEnabled() const { return get(Config::DiscordEnabled); }
    void setDiscordEnabled(bool v);

    QString themeName() const   { return get(Config::ThemeName); }
    void setThemeName(const QString &v);

    QString accentColor() const { return get(Config::AccentColor); }
    void setAccentColor(const QString &v);

    double uiScale() const      { return get(Config::UiScale); }
    void setUiScale(double v);

    static QString getTempDir();
    QMap<QString, QString> getGroupMap(const QString &group) const;

signals:
    void mpvLogEnabledChanged();
    void mpvYtdlEnabledChanged();
    void proxyChanged();
    void downloadDirChanged();
    void maxSpeedChanged();
    void subFontSizeChanged();
    void subPosChanged();
    void preferDubChanged();
    void aniskipEnabledChanged();
    void aniskipAutoChanged();
    void watchedPercentChanged();
    void danmakuEnabledChanged();
    void danmakuOpacityChanged();
    void danmakuFontScaleChanged();
    void danmakuSpeedChanged();
    void danmakuAreaChanged();
    void danmakuMinWeightChanged();
    void danmakuMaxOnScreenChanged();
    void danmakuBoldChanged();
    void danmakuOutlineChanged();
    void danmakuBlockScrollChanged();
    void danmakuBlockTopChanged();
    void danmakuBlockBottomChanged();
    void danmakuBlockColourChanged();
    void danmakuBlockRepeatChanged();
    void discordEnabledChanged();
    void themeNameChanged();
    void accentColorChanged();
    void uiScaleChanged();
    void settingsChanged();

private:
    void applyProxySettings(const QString &proxyString);
    // Coalesce disk flushes from frequent setters (e.g. volume/speed slider drags).
    void scheduleSync();
    mutable QSettings m_settings;
    QFileSystemWatcher m_fileWatcher;
    QTimer m_syncTimer;
    static inline std::atomic<bool> s_preferDub{false};
};

DECLARE_QML_SINGLETON(Settings);
