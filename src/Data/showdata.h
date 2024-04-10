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
    };
    inline PlaylistItem *getPlaylist() const { return playlist; }
    inline ShowProvider *getProvider() const { return provider; }


    friend class ShowManager;
    void setListType(int newListType) { listType = newListType; }

    void addEpisode(float number, const QString &link, const QString &name);
    QJsonObject toJson() const;
    QString toString() const;
private:
    int listType = -1;
    PlaylistItem* playlist = nullptr;
private:
    void copyFrom(const ShowData& other);
};


