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



struct Track {
    Track(const QUrl &url, const QString &title = "", const QString &lang = "")
        : url(url), title(title) {}
    QUrl url;
    QString title;
    QString lang;
    // mimetype, language
};

struct Video : public Track {
    Video(const QUrl &url, const QString &title = "", int resolution = 0, int bitrate = 0, const QString &lang = "")
        : Track(url, title, lang), resolution(resolution), bitrate(bitrate) {}

    int resolution;
    int bitrate;
    //bandwidth, framerate, mimetype
};

// struct Track {
//     Track(const QUrl &url, const QString &title = "", const QString &lang = "")
//         : url(url), title(title), lang(lang) {}
//     QUrl url;
//     QString title;
//     QString lang;
// };


class ShowProvider;
struct PlayItem {
    QList<Video> videos;
    QList<Track> audios;
    QList<Track> subtitles;
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




