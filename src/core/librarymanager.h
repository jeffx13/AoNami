#pragma once
#include <QAbstractListModel>
#include <QStandardItemModel>
#include <QDir>
#include <QCoreApplication>
#include <QtConcurrent/QtConcurrentRun>
#include "showdata.h"
#include "libraryproxymodel.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileSystemWatcher>



class LibraryManager: public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int                listType  READ getCurrentListType WRITE setDisplayingListType NOTIFY layoutChanged)
    Q_PROPERTY(LibraryProxyModel*  model    READ getProxyModel      CONSTANT)
public:
    explicit LibraryManager(QObject *parent = nullptr): QAbstractListModel(parent) {
        connect (&m_watchListFileWatcher, &QFileSystemWatcher::fileChanged, this, &LibraryManager::loadFile);
        m_proxyModel.setSourceModel(this);
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
    int getCurrentListType() const { return m_currentListType; }

    void updateLastWatchedIndex(const QString &showLink, int lastWatchedIndex, int timeStamp);

    ShowData::LastWatchInfo getLastWatchInfo(const QString& showLink);

    QJsonObject getShowJsonAt(int index, bool mapped = true) const;
    LibraryProxyModel* getProxyModel() { return &m_proxyModel; }

    Q_INVOKABLE bool loadFile(const QString &filePath = "");
    Q_INVOKABLE void cycleDisplayingListType();
    Q_INVOKABLE void changeListTypeAt(int index, int newListType, int oldListType = -1, bool isAbsoluteIndex = false);
    void changeListType(const QString &link, int newListType) {
        if (!m_showHashmap.contains(link)) return;
        auto showLibInfo = m_showHashmap.value(link);
        changeListTypeAt(showLibInfo.index, newListType, showLibInfo.listType, true);
    }

    Q_INVOKABLE void removeAt(int index, int listType = -1, bool isAbsoluteIndex = false);
    Q_INVOKABLE void move(int from, int to);
    void add(ShowData& show, int listType);

    void remove(const QString &link) {
        if (!m_showHashmap.contains(link)) return;

        // Extract list type and index from the hashmap
        auto libInfo = m_showHashmap.value(link);
        removeAt(libInfo.index, libInfo.listType, true);
    }
    Q_INVOKABLE void fetchUnwatchedEpisodes(int listType);

    void updateShowCover(const ShowData& show);



    Q_INVOKABLE int getListType(const QString &showLink) {
        if (!m_showHashmap.contains(showLink)) return -1;
        return m_showHashmap.value(showLink).listType;
    }

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


    int m_currentListType = WATCHING;
    void save();

    void updateProperty(const QString& showLink, const QList<Property>& properties);

    void setDisplayingListType(int newType) {
        if (newType == m_currentListType) return;
        m_currentListType = newType;
        emit layoutChanged();
    }

    LibraryProxyModel m_proxyModel;
    QFuture<void> m_fetchUnwatchedEpisodesJob;
    std::atomic<bool> m_isCancelled = false;
private:
    enum{
        TitleRole = Qt::UserRole,
        CoverRole,
        UnwatchedEpisodesRole,
        TypeRole,
    };
    int rowCount(const QModelIndex &parent) const override ;
    QVariant data(const QModelIndex &index, int role) const override ;
    QHash<int, QByteArray> roleNames() const override ;

};

