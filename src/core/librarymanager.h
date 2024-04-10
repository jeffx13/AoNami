#pragma once
#include <QAbstractListModel>
#include <QStandardItemModel>
#include <QDir>
#include <QCoreApplication>
#include <QtConcurrent>
#include "data/showdata.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

class LibraryManager: public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int      listType                READ getCurrentListType     WRITE setDisplayingListType NOTIFY layoutChanged)
    Q_PROPERTY(bool     isLoading               READ isLoading                                          NOTIFY isLoadingChanged)

    bool isLoading() { return m_isLoading; }
    bool m_isLoading = false;
    QFutureWatcher<void> m_watcher;

public:
    explicit LibraryManager(QObject *parent = nullptr): QAbstractListModel(parent) {}
    enum ListType {
        WATCHING,
        PLANNED,
        ON_HOLD,
        DROPPED,
        COMPLETED
    };
    //Q_ENUM(ListType);

    enum PropertyType {
        INT,
        STRING
    };

    inline void updateLastWatchedIndex(const QString& showLink, int lastWatchedIndex) {
        updateProperty(showLink, "lastWatchedIndex", lastWatchedIndex, INT);
        updateProperty(showLink, "timeStamp", 0, INT);
    }
    inline void updateTimeStamp(const QString& showLink, int timeStamp) {
        updateProperty(showLink, "timeStamp", timeStamp, INT);
    }
    void updateProperty(const QString& showLink, QString propertyName, QVariant propertyValue, PropertyType propertyType);
    ShowData::LastWatchInfo getLastWatchInfo(const QString& showLink);

    int getCurrentListType() const { return m_currentListType; }
    void setDisplayingListType(int listType) {
        if (listType == m_currentListType) return;
        m_currentListType = listType;
        emit layoutChanged();
    }
    void changeShowListType(ShowData& show, int newListType);
    QJsonObject getShowJsonAt(int index);
    Q_INVOKABLE void cycleDisplayingListType();
    Q_INVOKABLE void changeListTypeAt(int index, int newListType, int oldListType = -1);
    Q_INVOKABLE void removeAt(int index, int listType = -1);
    Q_INVOKABLE void move(int from, int to);
    void add(ShowData& show, int listType);
    void remove(ShowData& show);
    Q_INVOKABLE bool loadFile(const QString &filePath = "");
private:

    QString watchListFilePath;
    QMutex mutex;
    QJsonArray m_watchListJson;
    QHash<QString, QPair<int, int>> m_showHashmap;
    // QHash<QString, int> totalEpisodeMap; //TODO
    int m_currentListType = WATCHING;

    void save();
    void fetchUnwatchedEpisodes(int listType);


signals:
    void isLoadingChanged(void);
private:
    enum{
        TitleRole = Qt::UserRole,
        CoverRole,
        UnwatchedEpisodesRole
    };
    int rowCount(const QModelIndex &parent) const override ;
    QVariant data(const QModelIndex &index, int role) const override ;
    QHash<int, QByteArray> roleNames() const override ;
};

