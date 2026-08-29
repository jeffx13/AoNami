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
        double progress = 0.0;
        QSharedPointer<PlaylistItem> playlist;
    };

    void setPlaylist(QSharedPointer<PlaylistItem> playlist);
    QSharedPointer<PlaylistItem> getPlaylist() const { return m_playlist; }

    void addEpisode(int seasonNumber, float number, const QString &link, const QString &name, bool preview = false);

    bool isEmpty() const { return title.isEmpty() && link.isEmpty(); }
    void clear();

private:
    QSharedPointer<PlaylistItem> m_playlist;
};
