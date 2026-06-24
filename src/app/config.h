#pragma once
#include <QString>

// Single source of truth for persisted settings; typed Keys carry path + default.
namespace Config {

template <typename T>
struct Key {
    const char *path;
    T defaultValue;
};

// Player
inline const Key<int>     Volume      {"player/volume", 100};
inline const Key<double>  Speed       {"player/speed", 1.0};
inline const Key<bool>    Ytdl        {"player/ytdl", false};
inline const Key<int>     SubFontSize {"player/subFontSize", 40};
inline const Key<int>     SubPos      {"player/subPos", 100};
inline const Key<bool>    PreferDub   {"player/preferDub", false};  // false = prefer subbed
inline const Key<bool>    AniSkip     {"player/aniskip", true};     // auto-fetch AniSkip timestamps
inline const Key<bool>    AniSkipAuto {"player/aniskipAuto", false}; // auto-jump OP/ED using AniSkip data
inline const Key<int>     WatchedPercent {"player/watchedPercent", 80};  // % of an episode watched before it counts as complete

// Intro/outro skip - global manual fallback (used when a show has no saved profile)
inline const Key<int>     SkipOPStart  {"skip/fallbackOPStart", 0};
inline const Key<int>     SkipOPLength {"skip/fallbackOPLength", 90};
inline const Key<int>     SkipEDLength {"skip/fallbackEDLength", 90};

// Appearance
inline const Key<QString> ThemeName   {"ui/theme", QStringLiteral("obsidian")};
inline const Key<QString> AccentColor {"ui/accent", QString()};   // empty = use the theme's own accent
inline const Key<double>  UiScale     {"ui/scale", 1.0};           // global font/UI scale

// Logging
inline const Key<bool>    MpvLog      {"logging/mpv", true};

// Network
inline const Key<QString> Proxy         {"network/proxy", QString()};
inline const Key<bool>    LimitCache    {"network/limit_cache", false};
inline const Key<qint64>  ForwardCache  {"network/forward_cache", 0};
inline const Key<qint64>  BackwardCache {"network/backward_cache", 0};

// Download (download/dir has a runtime default - see Settings::downloadDir)
inline const Key<QString> MaxSpeed      {"download/maxSpeed", QString()};

// Discord - Client ID is a public app identifier (safe to ship); toggle gates presence.
inline const Key<bool>    DiscordEnabled  {"discord/enabled", true};
inline const Key<QString> DiscordClientId {"discord/clientId", QStringLiteral("1518260245552566283")};

// Per-show intro/outro skip profile (dynamic path keyed by show link)
inline QString skipProfile(const QString &showLink) {
    return QStringLiteral("skip/") + QString::number(qHash(showLink));
}

// Per-show MAL id override - remembers a manual AniSkip match when the auto one was wrong.
inline QString skipMal(const QString &showLink) {
    return QStringLiteral("skipmal/") + QString::number(qHash(showLink));
}

}  // namespace Config
