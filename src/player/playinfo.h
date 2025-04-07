#pragma once

#include <QString>
#include <QUrl>
#include <QMap>

#include <network/network.h>

struct VideoServer {
    QString name;
    QString link;
    struct SkipData {
        unsigned int introBegin;
        unsigned int introEnd;
        unsigned int outroBegin;
        unsigned int outroEnd;
    };
    std::optional<SkipData> skipData;
    VideoServer(const QString& name, const QString& link):name(name),link(link){}
};

struct Video {
    Video(const QString &url, const QString &label = "Video", int resolution = 0, int bitrate = 0)
        : url(url), label(label), resolution(resolution), bitrate(bitrate) {}

    QUrl url;
    QString label;
    int resolution = 0;
    int bitrate = 0;
    //bandwidth, framerate, mimetype
};

struct AudioTrack {
    AudioTrack(const QString &url, const QString &label = "Audio")
        : url(url), label(label) {}
    QUrl url;
    QString label;
    // mimetype, language
};

struct SubTrack {
    QString label;
    QUrl filePath;
};


class ShowProvider;
struct PlayItem {
    QList<Video> videos;
    QList<AudioTrack> audios;
    QList<SubTrack> subtitles;
    // ShowProvider *provider;
    QMap<QString, QString> headers;

    // int serverIndex = -1;
    int timeStamp = 0;

    void addHeader(const QString &key, const QString &value) {
        headers[key] = value;
    }

    void clear() {
        videos.clear();
        audios.clear();
        subtitles.clear();
        headers.clear();
    }
    // QString getHeaders(const QString &keyValueSeparator, const QString &entrySeparator, bool quotedValue = false) const {
    //     QString result;
    //     if (headers.isEmpty()) return result;
    //     QMapIterator<QString, QString> it(headers);
    //     bool first = true;
    //     while (it.hasNext()) {
    //         it.next();
    //         if (!first) {
    //             result += entrySeparator; // Add separator except before the first element
    //         } else {
    //             first = false;
    //         }
    //         auto value = quotedValue ? QString("\"%1\"").arg (it.value()) : it.value();
    //         result += it.key() + keyValueSeparator + value; // Append "key: value"
    //     }
    //     return result;
    // }
private:

};




