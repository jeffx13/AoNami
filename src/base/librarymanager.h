#pragma once
#include <QAbstractListModel>
#include <QStandardItemModel>
#include <QDir>
#include <QCoreApplication>
#include <QtConcurrent/QtConcurrentRun>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileSystemWatcher>

#include "servicemanager.h"
#include "showdata.h"
#include "gui/libraryproxymodel.h"



class LibraryManager: public ServiceManager
{
    Q_OBJECT
    Q_PROPERTY(int listType  READ getCurrentListType WRITE setDisplayingListType NOTIFY cleared)
public:
    explicit LibraryManager(QObject *parent = nullptr): ServiceManager(parent) {
        connect (&m_watchListFileWatcher, &QFileSystemWatcher::fileChanged, this, &LibraryManager::loadFile);
    }
    ~LibraryManager() {
        m_isCancelled = true;
        m_fetchUnwatchedEpisodesJob.waitForFinished();
    }

    enum ListType {
        WATCHING,
        PLANNED,
        PAUSED,
        DROPPED,
        COMPLETED
    };

    int count(int listType = -1) const;

    Q_INVOKABLE bool loadFile(const QString &filePath = "");
    Q_INVOKABLE void cycleDisplayingListType() { setDisplayingListType((m_currentListType + 1) % 5); }
    Q_INVOKABLE void changeListTypeAt(int index, int newListType, int oldListType = -1);
    void changeListType(const QString &link, int newListType);

    void add(ShowData& show, int listType);
    Q_INVOKABLE void removeAt(int index, int listType = -1);
    Q_INVOKABLE void removeByLink(const QString &link);
    Q_INVOKABLE void move(int from, int to);

    void updateProgress(const QString &showLink, int lastWatchedIndex, int timeStamp);
    ShowData::LastWatchInfo getLastWatchInfo(const QString& showLink);
    QJsonObject getShowJsonAt(int index) const;
    Q_INVOKABLE void fetchUnwatchedEpisodes(int listType);
    void updateShowCover(const ShowData& show);

    Q_INVOKABLE int getListType(const QString &showLink) const { return m_showHashmap.contains(showLink) ? m_showHashmap[showLink].listType : -1; }
    int getCurrentListType() const { return m_currentListType; }
    void setDisplayingListType(int newListType);

    int getTotalEpisodes(const QString &showLink) const { return m_showHashmap[showLink].totalEpisodes; }

    Q_SIGNAL void appended(int startIndex, int count);
    Q_SIGNAL void moved(int fromIndex, int toIndex);
    Q_SIGNAL void removed(int index);
    Q_SIGNAL void cleared(int oldCount);
    Q_SIGNAL void changed(int index);
    Q_SIGNAL void fetchedAllEpCounts();

private:
    struct Property {
        enum Type {
            INT,
            FLOAT,
            STRING
        };
        QString name;
        QVariant value;
        Type type;
    };
    const QString m_defaultLibraryPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + QDir::separator() + ".library");

    char m_updatedByApp = false;
    QString m_currentLibraryPath;
    QFileSystemWatcher m_watchListFileWatcher;
    QMutex mutex;
    QJsonArray m_watchListJson;


    struct ShowLibInfo {
        int listType;
        int index;
        int totalEpisodes;
    };
    QHash<QString, ShowLibInfo> m_showHashmap;

    void save();
    void updateProperty(const QString& showLink, const QList<Property>& properties);
private:
    int m_currentListType = WATCHING;
    QFuture<void> m_fetchUnwatchedEpisodesJob;


};

