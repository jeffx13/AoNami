#pragma once
#include <QObject>
#include <QSettings>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QVariant>
#include <QMap>
#include <QString>
#include <atomic>
#include "app/qml_singleton.h"
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
    Q_PROPERTY(int     watchedPercent  READ watchedPercent  WRITE setWatchedPercent  NOTIFY watchedPercentChanged)
    Q_PROPERTY(bool    discordEnabled  READ discordEnabled  WRITE setDiscordEnabled  NOTIFY discordEnabledChanged)
    Q_PROPERTY(QString path            READ getPath         CONSTANT)
    Q_PROPERTY(QString appDir          READ appDir          CONSTANT)

public:
    explicit Settings(QObject *parent = nullptr);
    static Settings& instance();

    // Typed access over the Config schema - preferred over the string overloads.
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

    int watchedPercent() const  { return get(Config::WatchedPercent); }
    void setWatchedPercent(int v);

    bool discordEnabled() const { return get(Config::DiscordEnabled); }
    void setDiscordEnabled(bool v);

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
    void watchedPercentChanged();
    void discordEnabledChanged();
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
