#ifndef SHOWRESPONSE_H
#define SHOWRESPONSE_H
#include "episode.h"

#include <QMetaType>

#include <QDebug>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QVector>
#include <iostream>

template <typename E>
constexpr typename std::underlying_type<E>::type to_underlying(E e) noexcept {
    return static_cast<typename std::underlying_type<E>::type>(e);
}

enum class TvType {
    Movie,
    TvSeries,
    Reality,
    Anime,
    Documentary
};
enum class Status {
    Ongoing,
    Completed
};
enum Providers{
    e_Nivod,
    e_Consumet9anime,
    e_Gogoanime
};


struct ShowResponse
{
    Q_GADGET
    Q_PROPERTY(QString title READ getTitle);
    Q_PROPERTY(QString coverUrl READ getCoverUrl);
    Q_PROPERTY(QString desc READ getDesc);
    Q_PROPERTY(QString year READ getYear);
    Q_PROPERTY(QString status READ getStatus);
    Q_PROPERTY(QString updateTime READ getUpdateTime);
    Q_PROPERTY(QString rating READ getRating);
    Q_PROPERTY(int views READ getViews);
    Q_PROPERTY(QString genresString READ getGenresString);
    Q_PROPERTY(bool isInWatchList READ getIsInWatchList);
    Q_PROPERTY(int lastWatchedIndex READ getLastWatchedIndex);
public:
    ShowResponse(QString title,QString link,QString coverUrl,int provider){
        this->title=title;
        this->link=link;
        this->coverUrl=coverUrl;
        this->provider=provider;
    };
    ShowResponse();
    QString title = "";
    QString link = "";
    QString coverUrl = "";
    int provider = 0;
    QString latestTxt = "";
    int type = 0;
    QVector<Episode> episodes;
    QString description = "";
    QString releaseDate = "";
    QString status = "Unknown";
    QVector<QString> genres{};
    QString updateTime = "Unknown";
    QString rating = "0.0";
    int views = 0;
    friend QDebug operator<<(QDebug debug, const ShowResponse& show)
    {
        debug << "Title: " << show.title << Qt::endl
              << "Link: " << show.link << Qt::endl
              << "Cover URL: " << show.coverUrl << Qt::endl
              << "Provider: " << show.provider << Qt::endl
              << "Latest Text: " << show.latestTxt << Qt::endl
              << "Type: " << show.type << Qt::endl
              << "Episodes: " << show.episodes.size ()<< Qt::endl
              << "Description: " << show.description << Qt::endl
              << "Year: " << show.releaseDate << Qt::endl
              << "Status: " << show.status << Qt::endl
              << "Genres: " << show.genres << Qt::endl
              << "Update Time: " << show.updateTime << Qt::endl
              << "Rating: " << show.rating << Qt::endl
              << "Views: " << show.views << Qt::endl;
        return debug;
    }
    friend QDebug operator<<(QDebug debug, const ShowResponse* show){
        debug << *show;
        return debug;
    }
    int getLastWatchedIndex() const {return lastWatchedIndex;}
    bool getIsInWatchList() const {return isInWatchList;}
    void setLastWatchedIndex(int index) { lastWatchedIndex = index;}
    void setIsInWatchList(bool isInWatchList) {this->isInWatchList=isInWatchList;}
    friend class ShowResponseObject;
private:
    bool isInWatchList = false;
    int lastWatchedIndex = -1;

    QString getTitle() const {return title;}
    QString getCoverUrl() const {return coverUrl;}
    QString getDesc() const {return description;}
    QString getYear() const {return releaseDate;}
    QString getUpdateTime() const {return updateTime;}
    QString getRating() const {return rating;}
    int getViews() const {return views;}
    QString getStatus() const {return status;}
    QString getGenresString() const {return genres.join (' ');}

};

class ShowResponseObject:public QObject{
    Q_OBJECT
    ShowResponse show;
public:
    ShowResponseObject(QObject* parent = nullptr) : QObject(parent) {}

    void setLastWatchedIndex(int index) {
        show.lastWatchedIndex = index;
        emit showPropertyChanged();
        emit lastWatchedIndexChanged();
    }

    void setIsInWatchList(bool isInWatchList) {
        show.isInWatchList = isInWatchList;
        emit showPropertyChanged();
    }

    void setShow(const ShowResponse& show) {this->show=show; emit showChanged();}

    ShowResponse* getShow() {return &show;}

    void emitPropertyChanged(){
        emit showPropertyChanged ();
    }
signals:
    void showChanged(void);
    void showPropertyChanged(void);
    void lastWatchedIndexChanged(void);
};

Q_DECLARE_METATYPE(ShowResponse)
#endif // SHOWRESPONSE_H
