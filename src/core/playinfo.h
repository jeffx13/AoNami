#pragma once
#include <QString>
#include <QUrl>
#include <QMap>
#include <QList>
#include <optional>

// VideoServer - represents an available server/source for an episode

struct VideoServer {
    // Sub/Dub steer the server selector; Unknown = the provider doesn't say.
    enum Translation { Unknown, Sub, Dub };

    QString name;
    QString link;
    Translation translation = Unknown;

    struct SkipData {
        unsigned int introBegin = 0;
        unsigned int introEnd = 0;
        unsigned int outroBegin = 0;
        unsigned int outroEnd = 0;
    };
    std::optional<SkipData> skipData;

    VideoServer(const QString& name, const QString& link, Translation translation = Unknown)
        : name(name), link(link), translation(translation) {}
};

// Track - a media stream (audio, subtitle, or base for video)

struct Track {
    QUrl url;
    QString title;
    QString lang;
    int bitrate = 0;  // bits/sec - used for sorting audio/video quality

    Track(const QUrl& url, const QString& title = "", const QString& lang = "", int bitrate = 0)
        : url(url), title(title), lang(lang), bitrate(bitrate) {}

    // Format bits/sec to human-readable: "320 kbps", "5.2 Mbps"
    static QString formatBitrate(int bps) {
        if (bps <= 0) return {};
        if (bps >= 1000000)
            return QString("%1 Mbps").arg(bps / 1000000.0, 0, 'f', 1);
        return QString("%1 kbps").arg(bps / 1000);
    }

    static QString detectLang(const QString& title) {
        if (title.isEmpty()) return {};

        struct LangEntry { const char *name; const char *code; };
        static constexpr LangEntry langMap[] = {
            { "english",    "en" }, { "chinese",    "zh" },
            { "japanese",   "ja" }, { "thai",       "th" },
            { "french",     "fr" }, { "spanish",    "es" },
            { "german",     "de" }, { "italian",    "it" },
            { "russian",    "ru" }, { "korean",     "ko" },
            { "arabic",     "ar" }, { "portuguese", "pt" },
            { "polish",     "pl" }, { "dutch",      "nl" },
            { "swedish",    "sv" }, { "norwegian",  "no" },
            { "danish",     "da" }, { "finnish",    "fi" },
            { "turkish",    "tr" }, { "czech",      "cs" },
            { "greek",      "el" }, { "hungarian",  "hu" },
            { "romanian",   "ro" }, { "hebrew",     "he" },
            { "slovak",     "sk" }, { "indonesian", "id" },
            { "malay",      "ms" }, { "vietnamese", "vi" },
            { "hindi",      "hi" },
        };

        const QString lowerTitle = title.toLower();
        for (const auto &entry : langMap) {
            if (lowerTitle.contains(QLatin1String(entry.name)))
                return QLatin1String(entry.code);
        }
        return {};
    }
};

// Video - a video stream with resolution metadata

struct Video : public Track {
    int resolution = 0;

    Video(const QUrl& url, const QString& title = "", int resolution = 0,
          int bitrate = 0, const QString& lang = "")
        : Track(url, title, lang, bitrate), resolution(resolution) {}
};

// PlayInfo - complete playback information for an episode

struct PlayInfo {
    QList<Video> videos;
    QList<Track> audios;
    QList<Track> subtitles;
    QMap<QString, QString> headers;
    int timestamp = 0;

    void addHeader(const QString& key, const QString& value) {
        headers.insert(key, value);
    }

    void clear() {
        videos.clear();
        audios.clear();
        subtitles.clear();
        headers.clear();
        timestamp = 0;
    }

    bool hasVideo() const { return !videos.isEmpty(); }
};
