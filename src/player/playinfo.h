#pragma once

#include <QString>
#include <QUrl>
#include <QHash>

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
    Video(QUrl videoUrl) : videoUrl(videoUrl) {}
    QUrl videoUrl;
    QString resolution = "N/A";
    QUrl audioUrl;

    void addHeader(const QString &key, const QString &value) {
        m_headers[key] = value;
    }

    QString getHeaders(const QString &keyValueSeparator, const QString &entrySeparator, bool quotedValue = false) const {
        QString result;
        if (m_headers.isEmpty()) return result;
        QHashIterator<QString, QString> it(m_headers);
        bool first = true;
        while (it.hasNext()) {
            it.next();
            if (!first) {
                result += entrySeparator; // Add separator except before the first element
            } else {
                first = false;
            }
            auto value = quotedValue ? QString("\"%1\"").arg (it.value()) : it.value();
            result += it.key() + keyValueSeparator + value; // Append "key: value"
        }
        return result;
    }
    QHash<QString, QString> getHeaders() const {
        return m_headers;
    }
private:
    QHash<QString, QString> m_headers {
        {"User-Agent", "Mozilla/5.0 (Linux; Android 8.0.0; moto g(6) play Build/OPP27.91-87) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/69.0.3497.100 Mobile Safari/537.36"}
    };
};

struct SubTrack {
    QString label;
    QUrl filePath;
};

struct PlayInfo {
    QVector<Video> sources;
    QVector<SubTrack> subtitles;
    int serverIndex = -1;
};




