#pragma once
#include <QAbstractListModel>
#include <QStandardItemModel>
#include <QDir>
#include <QCoreApplication>
#include <QtConcurrent>
#include "showdata.h"
#include "libraryproxymodel.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>


class LibraryManager: public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int                listType  READ getCurrentListType WRITE setDisplayingListType NOTIFY layoutChanged)
    Q_PROPERTY(bool               isLoading READ isLoading                                      NOTIFY isLoadingChanged)
    Q_PROPERTY(LibraryProxyModel*  model     READ getProxyModel      CONSTANT)


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
    LibraryProxyModel m_proxyModel;
public:
    explicit LibraryManager(QObject *parent = nullptr): QAbstractListModel(parent) {
        connect (&m_watchListFileWatcher, &QFileSystemWatcher::fileChanged, this, &LibraryManager::loadFile);
        // connect(&debounceTimer, &QTimer::timeout, this, [&](){
        //     m_fileChangeCooldown = false;
        // });
        // debounceTimer.setSingleShot(true);
        // debounceTimer.setInterval(100); // Adjust the interval as needed

        m_proxyModel.setSourceModel(this);
    }
    const QString m_defaultLibraryPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + QDir::separator() + ".library");


    enum ListType {
        WATCHING,
        PLANNED,
        ON_HOLD,
        DROPPED,
        COMPLETED
    };

    void updateLastWatchedIndex(const QString &showLink, int lastWatchedIndex);

    void updateTimeStamp(const QString &showLink, int timeStamp);

    ShowData::LastWatchInfo getLastWatchInfo(const QString& showLink);

    int getCurrentListType() const { return m_currentListType; }

    QJsonObject getShowJsonAt(int index, bool mapped = true) const;

    Q_INVOKABLE bool loadFile(const QString &filePath = "");
    Q_INVOKABLE void cycleDisplayingListType();
    Q_INVOKABLE void changeListTypeAt(int index, int newListType, int oldListType = -1);
    Q_INVOKABLE void removeAt(int index, int listType = -1);
    Q_INVOKABLE void move(int from, int to);
    void add(ShowData& show, int listType);
    void remove(ShowData& show);
    LibraryProxyModel* getProxyModel();


private:
    char m_updatedByApp = false;
    QString m_currentLibraryPath;
    QFileSystemWatcher m_watchListFileWatcher;
    QMutex mutex;
    QJsonArray m_watchListJson;
    QHash<QString, QPair<int, int>> m_showHashmap;
    // QHash<QString, int> totalEpisodeMap; //TODO
    int m_currentListType = WATCHING;
    void save();
    void fetchUnwatchedEpisodes(int listType);
    void updateProperty(const QString& showLink, const QList<Property>& properties);
    void changeShowListType(ShowData& show, int newListType);

    void setDisplayingListType(int listType) {
        if (listType == m_currentListType) return;
        m_currentListType = listType;
        emit layoutChanged();
    }

    bool isLoading() { return m_isLoading; }
    bool m_isLoading = false;
    Q_SIGNAL void isLoadingChanged(void);


private:
    enum{
        TitleRole = Qt::UserRole,
        CoverRole,
        TypeRole,
        UnwatchedEpisodesRole
    };
    int rowCount(const QModelIndex &parent) const override ;
    QVariant data(const QModelIndex &index, int role) const override ;
    QHash<int, QByteArray> roleNames() const override ;

};

