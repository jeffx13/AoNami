#pragma once

#include <QObject>
#include <QList>
#include <QJsonObject>
#include <QDebug>
#include <memory>
#include <QSharedPointer>
#include <QString>

class PlaylistItem;
class ShowProvider;

struct ShowData
{
    ShowData(const QString& title = "", const QString& link = "", const QString& coverUrl = "",
             ShowProvider* provider = nullptr, const QString& latestTxt = "", int type = 0)
        : title(title), link(link), coverUrl(coverUrl), provider(provider), latestTxt(latestTxt), type(type) {}

    static ShowData fromMap(const QVariantMap& showDataMap) {
        QString title = showDataMap["title"].toString();
        QString link = showDataMap["link"].toString();
        QString coverUrl = showDataMap["cover"].toString();
        int type = showDataMap["type"].toInt();
        return ShowData(title, link, coverUrl, nullptr, "", type);
    }

    // Rule of five - modern C++ approach
    ShowData(const ShowData& other) = default;
    ShowData(ShowData&& other) noexcept = default;
    ShowData& operator=(const ShowData& other) = default;
    ShowData& operator=(ShowData&& other) noexcept = default;
    ~ShowData() = default;

    // Member variables
    QString title;
    QString link;
    QString coverUrl;
    QString latestTxt;
    ShowProvider* provider = nullptr;
    QString description;
    QString releaseDate;
    QString status;
    QList<QString> genres;
    QString updateTime;
    QString score;
    QString views;
    int type = ShowData::NONE;

public:
    enum ShowType
    {
        NONE        = 0,
        ANIME       = 1,
        MOVIE       = 2,
        TVSERIES    = 3,
        VARIETY     = 4,
        DOCUMENTARY = 5,
    };

    enum Status
    {
        Ongoing,
        Completed
    };

    struct LastWatchInfo {
        int libraryType = -1;
        int lastWatchedIndex = -1;
        int timestamp = 0;
        QSharedPointer<PlaylistItem> playlist;
    };

    // Optimized methods
    void setPlaylist(QSharedPointer<PlaylistItem> playlist);
    QSharedPointer<PlaylistItem> getPlaylist() const { return m_playlist; }
    void addEpisode(int seasonNumber, float number, const QString &link, const QString &name);
    QString toString() const;

    // New optimized methods
    bool hasPlaylist() const { return m_playlist != nullptr; }
    bool isEmpty() const { return title.isEmpty() && link.isEmpty(); }
    void clear();
    void reserveEpisodes(size_t count);

private:
    QSharedPointer<PlaylistItem> m_playlist;
};

