#pragma once
#include <QString>
#include <QUrl>
#include <QMap>
#include <QList>

struct VideoServer {
    QString name;
    QString link;

    struct SkipData {
        unsigned int introBegin = 0;
        unsigned int introEnd = 0;
        unsigned int outroBegin = 0;
        unsigned int outroEnd = 0;
    };

    std::optional<SkipData> skipData;

    VideoServer(const QString& name, const QString& link)
        : name(name), link(link) {}
};

struct Track {
    QUrl url;
    QString title;
    QString lang;

    Track(const QUrl& url, const QString& title = "", const QString& lang = "")
        : url(url), title(title), lang(lang) {}

    static QString detectLang(const QString& title) {
        if (title.isEmpty()) return "";
        static const QMap<QString, QString> langMap = {
            { "english", "en" },
            { "chinese", "zh" },
            { "japanese", "ja" },
            { "thai", "th" },
            { "french", "fr" },
            { "spanish", "es" },
            { "german", "de" },
            { "italian", "it" },
            { "russian", "ru" },
            { "korean", "ko" },
            { "arabic", "ar" },
            { "portuguese", "pt" },
            { "polish", "pl" },
            { "dutch", "nl" },
            { "swedish", "sv" },
            { "norwegian", "no" },
            { "danish", "da" },
            { "finnish", "fi" },
            { "turkish", "tr" },
            { "czech", "cs" },
            { "greek", "el" },
            { "hungarian", "hu" },
            { "romanian", "ro" },
            { "hebrew", "he" },
            { "slovak", "sk" },
            { "indonesian", "id" },
            { "malay", "ms" },
            { "vietnamese", "vi" },
            { "hindi", "hi" }
        };
        QString lowerLabel = title.toLower();
        for (auto it = langMap.constBegin(); it != langMap.constEnd(); ++it) {
            if (lowerLabel.contains(it.key(), Qt::CaseInsensitive))
                return it.value();
        }
        return "";
    }
};

struct Video : public Track {
    int resolution = 0;
    int bitrate = 0;

    Video(const QUrl& url, const QString& title = "", int resolution = 0, int bitrate = 0, const QString& lang = "")
        : Track(url, title, lang), resolution(resolution), bitrate(bitrate) {}
};

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
};
