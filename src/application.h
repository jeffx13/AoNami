#pragma once
#include <QAbstractListModel>
#include <QObject>
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
private:
    Application(const Application &) = delete;
    Application &operator=(const Application &) = delete;

public:
    Q_INVOKABLE void search(const QString& query, int page);
    Q_INVOKABLE void latest(int page);
    Q_INVOKABLE void popular(int page);
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
    explicit Application(const QString &launchPath);
    ~Application();

private:

};
