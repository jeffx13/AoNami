#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QQmlApplicationEngine>
#include <qqmlintegration.h>
#include <QClipboard>
#include <QSharedMemory>

#include "gui/cursor.h"
#include "app/qml_singleton.h"

#include "base/downloadmanager.h"
#include "base/player/playlistmanager.h"
#include "base/librarymanager.h"
#include "base/providermanager.h"
#include "base/searchmanager.h"
#include "base/showmanager.h"

#include "gui/librarymodel.h"
#include "gui/libraryproxymodel.h"
#include "gui/playlistmodel.h"
#include "gui/searchresultmodel.h"

class Application: public QObject
{
    Q_OBJECT
    Q_PROPERTY(ProviderManager     *providerManager   READ getProviderManager   CONSTANT)
    Q_PROPERTY(ShowManager         *showManager       READ getShowManager       CONSTANT)
    Q_PROPERTY(SearchManager       *explorer          READ getSearchManager     CONSTANT)
    Q_PROPERTY(SearchResultModel   *searchResultModel READ getSearchResultModel CONSTANT)
    Q_PROPERTY(LibraryManager      *library           READ getLibrary           CONSTANT)
    Q_PROPERTY(LibraryProxyModel   *libraryModel      READ getLibraryModel      CONSTANT)
    Q_PROPERTY(PlaylistManager     *play              READ getPlaylist          CONSTANT)
    Q_PROPERTY(PlaylistModel       *playlistModel     READ getPlaylistModel     CONSTANT)
    Q_PROPERTY(DownloadManager     *downloader        READ getDownloader        CONSTANT)
    Q_PROPERTY(Cursor              *cursor            READ getCursor            CONSTANT)
    Q_PROPERTY(LogListModel        *logList           READ getLogList           CONSTANT)

private:
    ProviderManager     *getProviderManager()      { return &m_providerManager;     }
    ShowManager         *getShowManager()          { return &m_showManager;         }
    SearchManager       *getSearchManager()        { return &m_searchManager;       }
    LibraryManager      *getLibrary()              { return &m_libraryManager;      }
    PlaylistManager     *getPlaylist()             { return &m_playlistManager;     }
    PlaylistModel       *getPlaylistModel()        { return &m_playlistModel;       }
    DownloadManager     *getDownloader()           { return &m_downloadManager;     }
    Cursor              *getCursor()               { return &m_cursor;              }
    LogListModel        *getLogList()              { return &QLog::logListModel;    }
    SearchResultModel   *getSearchResultModel()    { return &m_searchResultModel;   }
    LibraryProxyModel   *getLibraryModel()         { return &m_libraryProxyModel;   }

    SearchManager       m_searchManager;
    SearchResultModel   m_searchResultModel;

    PlaylistManager     m_playlistManager;
    PlaylistModel       m_playlistModel;

    LibraryManager      m_libraryManager;
    LibraryModel        m_libraryModel;
    LibraryProxyModel   m_libraryProxyModel;

    ProviderManager     m_providerManager{this};
    DownloadManager     m_downloadManager{this};
    Cursor              m_cursor         {this};
    ShowManager         m_showManager    {this};

public:
    Q_INVOKABLE void explore(const QString& query = QString(), int page = 0, bool isLatest = true);
    Q_INVOKABLE void exploreMore(bool isReload);
    Q_INVOKABLE void loadShow(int index, bool fromWatchList);
    Q_INVOKABLE void playFromEpisodeList(int index, bool append);
    Q_INVOKABLE void continueWatching();

    Q_INVOKABLE void addToLibrary(int index, int libraryType) { m_libraryManager.add(index == -1 ? m_showManager.getShow() : m_searchManager.getResultAt(index), libraryType); }
    Q_INVOKABLE void appendToPlaylists(int index, bool fromLibrary, bool play = false);
    Q_INVOKABLE void downloadCurrentShow(int startIndex, int count = 1);

    Q_INVOKABLE void copyToClipboard(const QString &text) { QGuiApplication::clipboard()->setText(text); }
public:
    Application(Application &&) = delete;
    Application &operator=(Application &&) = delete;
    explicit Application(const QString &launchPath);
    ~Application();
    void setFont(QString fontPath);
private:
    Application(const Application &) = delete;
    Application &operator=(const Application &) = delete;
    std::function<void(bool)> m_lastSearch;
    void setOneInstance();
};

DECLARE_QML_NAMED_SINGLETON(Application, App)


