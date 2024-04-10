#pragma once

#include <QString>
#include <QUrl>
#include <QHash>


struct Video {
    Video(QUrl videoUrl) : videoUrl(videoUrl) {}
    QUrl videoUrl;
    QString resolution = "N/A";
    QList<QUrl> subtitles;
    QUrl audioUrl;
    void addHeader(const QString &key, const QString &value);
    // QString getMpvHeaders() const;
    QString getHeaders(const QString &keyValueSeparator = ": ", const QString &entrySeparator = ",", bool quotedValue = false) const;;
    // QString getDownloaderHeaders() const;;
private:
    QHash<QString, QString> m_headers {};


};

