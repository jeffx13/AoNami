#pragma once
#include <QAbstractListModel>
#include <QObject>
#include <QQmlApplicationEngine>
#include <qqmlintegration.h>
#include "utils/cursor.h"

#include "core/downloadmanager.h"
#include "player/playlistmanager.h"
#include "core/librarymanager.h"
#include "core/providermanager.h"
#include "core/searchresultmanager.h"
#include "core/showmanager.h"


class Application: public QObject
{
    Q_OBJECT
    Q_PROPERTY(ProviderManager     *providerManager READ getProviderManager      CONSTANT)
    Q_PROPERTY(ShowManager         *currentShow     READ getCurrentShow          CONSTANT)
    Q_PROPERTY(SearchResultManager *explorer        READ getSearchResultsManager CONSTANT)
    Q_PROPERTY(LibraryManager      *library         READ getLibrary              CONSTANT)
    Q_PROPERTY(PlaylistManager     *play            READ getPlaylist             CONSTANT)
    Q_PROPERTY(DownloadManager     *downloader      READ getDownloader           CONSTANT)
    Q_PROPERTY(Cursor              *cursor          READ getCursor               CONSTANT)
    QML_ELEMENT
    QML_SINGLETON
private:
    ProviderManager     *getProviderManager()      { return &m_providerManager;     }
    ShowManager         *getCurrentShow()          { return &m_showManager;         }
    SearchResultManager *getSearchResultsManager() { return &m_searchResultManager; }
    LibraryManager      *getLibrary()              { return &m_libraryManager;      }
    PlaylistManager     *getPlaylist()             { return &m_playlistManager;     }
    DownloadManager     *getDownloader()           { return &m_downloadManager;      }
    Cursor              *getCursor()               { return &m_cursor;              }

    ProviderManager     m_providerManager{this};
    SearchResultManager m_searchResultManager{this};
    LibraryManager      m_libraryManager{this};
    PlaylistManager     m_playlistManager{this};
    DownloadManager     m_downloadManager{this};
    Cursor              m_cursor{this};
    ShowManager         m_showManager{this};
    QGuiApplication &app;
public:
    Q_INVOKABLE void explore(const QString& query = QString(), int page = 0, bool isLatest = true){
        int type = m_providerManager.getCurrentSearchType();
        auto provider = m_providerManager.getCurrentSearchProvider();
        if (!query.isEmpty()) {
            m_searchResultManager.search (query, page, type, provider);
        } else if (isLatest){
            m_searchResultManager.latest(page, type, provider);
        } else {
            m_searchResultManager.popular(page, type, provider);
        }
        m_lastSearch = [this, query, isLatest, page](bool isReload = false) {
            explore(query, isReload ? page : page + 1);
        };
    }
    Q_INVOKABLE void exploreMore(bool isReload) {
        if (!isReload && !m_searchResultManager.canLoadMore()) return;
        m_lastSearch(isReload);
    }
    Q_INVOKABLE void loadShow(int index, bool fromWatchList);
    Q_INVOKABLE void playFromEpisodeList(int index);
    Q_INVOKABLE void continueWatching();
    Q_INVOKABLE void addCurrentShowToLibrary(int listType);
    Q_INVOKABLE void removeCurrentShowFromLibrary();
    Q_INVOKABLE void downloadCurrentShow(int startIndex, int count = 1);;
    Q_INVOKABLE void updateTimeStamp();
private slots:
    Q_SLOT void updateLastWatchedIndex();
public:
    Application(Application &&) = delete;
    Application &operator=(Application &&) = delete;
    explicit Application(QGuiApplication &app, const QString &launchPath,
                         QObject *parent = nullptr);
    ~Application();
    void setFont(QString fontPath);
    QQmlApplicationEngine engine;
private:
    Application(const Application &) = delete;
    Application &operator=(const Application &) = delete;
    std::function<void(bool)> m_lastSearch;
    void setOneInstance(){
        QSharedMemory shared("62d60669-bb94-4a94-88bb-b964890a7e04");
        if ( !shared.create( 512, QSharedMemory::ReadWrite) )
        {
            qWarning() << "Can't start more than one instance of the application.";
            exit(0);
        }
        else {
            qDebug() << "Application started successfully.";
        }
    }


};




