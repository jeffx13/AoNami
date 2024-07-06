#pragma once

#include <QObject>
#include <QList>
#include <QJsonObject>
#include <QDebug>
class PlaylistItem;
class ShowProvider;

struct ShowData
{
    ShowData(const QString& title, const QString& link, const QString& coverUrl,
             ShowProvider* provider, const QString& latestTxt = "", int type = 0)
        : title(title), link(link), coverUrl(coverUrl), provider(provider), latestTxt(latestTxt), type(type) {};

    ShowData (const ShowData& other);

    static ShowData fromJson(const QJsonObject& showJson, ShowProvider* provider = nullptr) {
        QString title = showJson["title"].toString();
        QString link = showJson["link"].toString();
        QString coverUrl = showJson["cover"].toString();
        int type = showJson["type"].toInt();
        return ShowData(title, link, coverUrl, provider, "", type);
    }

    ShowData(ShowData &&other);
    ShowData& operator=(const ShowData&& other) = delete;

    ShowData& operator=(ShowData&& other);
    ShowData& operator=(const ShowData& other);

    ~ShowData();;
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
    int type = 0;

public:

    enum ShowType
    {
        MOVIE = 1,
        TVSERIES,
        VARIETY,
        ANIME,
        DOCUMENTARY,
        NONE
    };
    enum Status
    {
        Ongoing,
        Completed
    };

    struct LastWatchInfo {
        int listType = -1;
        int lastWatchedIndex = -1;
        int timeStamp = 0;
        PlaylistItem *playlist;
    };
    void setPlaylist(PlaylistItem *playlist);
    inline PlaylistItem *getPlaylist() const { return m_playlist; }
    inline ShowProvider *getProvider() const { return provider; }
    inline void setListType(int listType) { m_listType = listType; }
    inline int getListType() const { return m_listType; }

    void addEpisode(float number, const QString &link, const QString &name);
    QJsonObject toJson() const;
    QString toString() const;
private:
    int m_listType = -1;
    PlaylistItem* m_playlist = nullptr;
private:
    void copyFrom(const ShowData& other);
};

