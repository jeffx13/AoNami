#pragma once

#include <QObject>
#include <QList>
#include <QJsonObject>
#include <QDebug>
class PlaylistItem;
class ShowProvider;

struct ShowData
{
    ShowData(const QString& title = "", const QString& link = "", const QString& coverUrl = "",
             ShowProvider* provider = nullptr, const QString& latestTxt = "", int type = 0)
        : title(title), link(link), coverUrl(coverUrl), provider(provider), latestTxt(latestTxt), type(type) {}

    // static ShowData fromJson(const QJsonObject& showJson, ShowProvider* provider = nullptr) {
    //     QString title = showJson["title"].toString();
    //     QString link = showJson["link"].toString();
    //     QString coverUrl = showJson["cover"].toString();
    //     int type = showJson["type"].toInt();
    //     return ShowData(title, link, coverUrl, provider, "", type);
    // }

    static ShowData fromMap(const QVariantMap& showDataMap) {
        QString title = showDataMap["title"].toString();
        QString link = showDataMap["link"].toString();
        QString coverUrl = showDataMap["cover"].toString();
        int type = showDataMap["type"].toInt();
        return ShowData(title, link, coverUrl, nullptr, "", type);
    }

    ShowData (const ShowData& other);
    ShowData& operator=(const ShowData& other);

    ~ShowData();
    QString title;
    QString link;
    QString coverUrl;
    QString latestTxt;
    ShowProvider* provider;
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
        PlaylistItem *playlist;
    };
    void setPlaylist(PlaylistItem *playlist);
    PlaylistItem *getPlaylist() const { return m_playlist; }
    void addEpisode(int seasonNumber, float number, const QString &link, const QString &name);
    QString toString() const;
private:
    PlaylistItem* m_playlist = nullptr;
    void copyFrom(const ShowData& other);
};

