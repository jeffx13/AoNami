#pragma once

#include <QObject>
#include <QGuiApplication>
#include <QClipboard>

#include "app/qml_singleton.h"
#include "show/searchmanager.h"
#include "providers/providermanager.h"
#include "show/showmanager.h"
#include "library/librarymanager.h"
#include "player/playlistmanager.h"
#include "player/skipmanager.h"
#include "presence/discordpresence.h"
#include "download/downloadmanager.h"

#include "library/libraryproxymodel.h"
#include "app/settings.h"

// Ties the managers together; methods live here only when 2+ of them are involved.

class Application : public QObject
{
    Q_OBJECT
    Q_PROPERTY(ProviderManager     *providerManager   READ getProviderManager   CONSTANT)
    Q_PROPERTY(ShowManager         *showManager       READ getShowManager       CONSTANT)
    Q_PROPERTY(SearchManager       *explorer          READ getSearchManager     CONSTANT)
    Q_PROPERTY(SearchManager       *searchResultModel READ getSearchResultModel CONSTANT)
    Q_PROPERTY(SearchManager       *trending          READ getTrending          CONSTANT)
    Q_PROPERTY(LibraryManager      *library           READ getLibrary           CONSTANT)
    Q_PROPERTY(LibraryProxyModel   *libraryModel      READ getLibraryModel      CONSTANT)
    Q_PROPERTY(PlaylistManager     *playlist          READ getPlaylist          CONSTANT)
    Q_PROPERTY(PlaylistManager     *playlistModel     READ getPlaylistModel     CONSTANT)
    Q_PROPERTY(SkipManager         *skip              READ getSkip              CONSTANT)
    Q_PROPERTY(DownloadManager     *downloader        READ getDownloader        CONSTANT)
    Q_PROPERTY(DownloadManager     *downloadListModel READ getDownloadListModel CONSTANT)
    Q_PROPERTY(LogListModel        *logList           READ getLogList           CONSTANT)
    Q_PROPERTY(Settings            *settings          READ getSettings          CONSTANT)

public:
    explicit Application(const QString &launchPath);
    ~Application();
    Application(const Application &) = delete;
    Application &operator=(const Application &) = delete;

    Q_INVOKABLE void explore(const QString &query = QString(), int page = 0, bool isLatest = true);
    Q_INVOKABLE void loadShow(int index, bool fromLibrary);
    Q_INVOKABLE void playFromEpisodeList(int index, bool append);
    Q_INVOKABLE void continueWatching();
    Q_INVOKABLE void addToLibrary(int index, int libraryType);
    Q_INVOKABLE void appendToPlaylists(int index, bool fromLibrary, bool play = false);
    Q_INVOKABLE void resumeFromLibrary(const QString &link);  // load by link, then auto-continue
    Q_INVOKABLE void resumeFromHistory(const QString &link);  // resume a History row (library or not)
    Q_INVOKABLE void loadTrending(int index);
    Q_INVOKABLE void appendTrending(int index, bool play = false);
    Q_INVOKABLE void downloadCurrentShow(int startIndex, int endIndex = -1);
    Q_INVOKABLE void copyToClipboard(const QString &text) { QGuiApplication::clipboard()->setText(text); }

    void setFont(const QString &fontPath);

private:
    void prefetchStartupData();
    void checkForUpdates();
    static bool isNewerVersion(const QString &latest, const QString &current);
    ProviderManager   *getProviderManager()   { return &m_providerManager;   }
    ShowManager       *getShowManager()       { return &m_showManager;       }
    SearchManager     *getSearchManager()     { return &m_searchManager;     }
    LibraryManager    *getLibrary()           { return &m_libraryManager;    }
    PlaylistManager   *getPlaylist()          { return &m_playlistManager;   }
    PlaylistManager   *getPlaylistModel()     { return &m_playlistManager;   }
    SkipManager       *getSkip()              { return &m_skipManager;       }
    DownloadManager   *getDownloader()        { return &m_downloadManager;   }
    DownloadManager   *getDownloadListModel() { return &m_downloadManager;   }
    LogListModel      *getLogList()           { return &QLog::logListModel;  }
    Settings          *getSettings()          { return &Settings::instance();}
    SearchManager     *getSearchResultModel() { return &m_searchManager;     }
    SearchManager     *getTrending()          { return &m_trendingManager;   }
    LibraryProxyModel *getLibraryModel()      { return &m_libraryProxyModel; }

    void loadResult(SearchManager &src, int index);
    void appendResult(SearchManager &src, int index, bool play);

    // Members (init order matters)
    SearchManager       m_searchManager;
    SearchManager       m_trendingManager;
    bool                m_pendingAutoResume = false;

    PlaylistManager     m_playlistManager;

    LibraryManager      m_libraryManager;
    LibraryProxyModel   m_libraryProxyModel;

    DownloadManager     m_downloadManager;

    ProviderManager     m_providerManager{this};
    ShowManager         m_showManager    {this};
    SkipManager         m_skipManager    {this};
    DiscordPresence     m_discordPresence{this};
};

DECLARE_QML_NAMED_SINGLETON(Application, App)