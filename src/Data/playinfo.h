#pragma once

#include <QString>
#include <QUrl>
#include <QHash>


struct Video {
    Video(QUrl videoUrl) : videoUrl(videoUrl) {}
    QUrl videoUrl;
    QString resolution = "N/A";
    QUrl audioUrl;

    void addHeader(const QString &key, const QString &value) {
        m_headers[key] = value;
    }

    QString getHeaders(const QString &keyValueSeparator = ": ", const QString &entrySeparator = ",", bool quotedValue = false) const {
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
private:
    QHash<QString, QString> m_headers {};
};

struct SubTrack {
    QString filePath;
    QString label;
};

struct PlayInfo {
    QVector<Video> sources;
    QVector<SubTrack> subtitles;
    int serverIndex = -1;
};




