#pragma once
#include <QObject>
#include <QList>
#include <QVariantMap>
#include <QSharedPointer>
#include <QString>

class PlaylistItem;
class ShowProvider;

struct ShowData
{
    ShowData(const QString &title = "", const QString &link = "", const QString &coverUrl = "",
             ShowProvider *provider = nullptr, const QString &latestTxt = "", int type = 0)
        : title(title), link(link), coverUrl(coverUrl), provider(provider), latestTxt(latestTxt), type(type) {}

    static ShowData fromMap(const QVariantMap &map) {
        return ShowData(
            map["title"].toString(),
            map["link"].toString(),
            map["cover"].toString(),
            nullptr, "",
            map["type"].toInt()
            );
    }

    // Defaulted move/copy
    ShowData(const ShowData &) = default;
    ShowData(ShowData &&) noexcept = default;
    ShowData &operator=(const ShowData &) = default;
    ShowData &operator=(ShowData &&) noexcept = default;
    ~ShowData() = default;

    QString title;
    QString link;
    QString coverUrl;
    QString latestTxt;
    ShowProvider *provider = nullptr;
    QString description;
    QString releaseDate;
    QString status;
    QList<QString> genres;
    QString updateTime;
    QString score;
    QString views;
    int type = NONE;

    enum ShowType { NONE = 0, ANIME = 1, MOVIE = 2, TVSERIES = 3, VARIETY = 4, DOCUMENTARY = 5 };

    struct LastWatchInfo {
        int libraryType = -1;
        int lastWatchedIndex = -1;
        int timestamp = 0;
        QSharedPointer<PlaylistItem> playlist;
    };

    void setPlaylist(QSharedPointer<PlaylistItem> playlist);
    QSharedPointer<PlaylistItem> getPlaylist() const { return m_playlist; }
    bool hasPlaylist() const { return m_playlist != nullptr; }

    void addEpisode(int seasonNumber, float number, const QString &link, const QString &name);
    void reserveEpisodes(size_t count);

    bool isEmpty() const { return title.isEmpty() && link.isEmpty(); }
    void clear();
    QString toString() const;

private:
    QSharedPointer<PlaylistItem> m_playlist;
};
