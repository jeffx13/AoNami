#include "app/application.h"
#include <QNetworkProxyFactory>
#include <QFontDatabase>
#include <QQuickStyle>
#include <QtConcurrent/QtConcurrentRun>
#include <libxml/parser.h>
#include "app/logger.h"
#include "app/settings.h"
#include "ui/uibridge.h"
#include "media/danmaku.h"
#include "net/client.h"
#include "net/hlsproxy.h"
#include "net/cloudflare.h"
#include "providers/providerregistry.h"

Application::Application(const QString &launchPath)
    : m_searchManager(this)
    , m_libraryManager(this)
    , m_libraryProxyModel(&m_libraryManager)
    , m_playlistManager(this)
    , m_downloadManager(this)
{
    REGISTER_QML_SINGLETON(Application, this);
    REGISTER_QML_SINGLETON(UiBridge, &UiBridge::instance());
    REGISTER_QML_SINGLETON(Settings, &Settings::instance());

    xmlInitParser();
    QNetworkProxyFactory::setUseSystemConfiguration(true);
    // Solves run on worker threads and outlive the window otherwise, holding the
    // process open with a browser still on screen.
    QObject::connect(qApp, &QCoreApplication::aboutToQuit, qApp, [] { Cloudflare::shutdown(); });
    new HlsProxy(this);
    DanmakuAss::pruneCache(Settings::getTempDir() + QStringLiteral("/danmaku"));
    m_libraryProxyModel.setSourceModel(&m_libraryManager);

    m_providerManager.setProviders(ProviderRegistry::createAll(this));

    if (!launchPath.isEmpty())
        m_playlistManager.openUrl(QUrl::fromUserInput(launchPath), false);

    connect(&m_playlistManager, &PlaylistManager::progressUpdated,
            &m_libraryManager,  &LibraryManager::updateProgress);

    connect(&m_playlistManager, &PlaylistManager::episodeStarted,
            &m_libraryManager, [this](const QString &link, int index, int timestamp) {
                m_libraryManager.recordHistory(link, index, timestamp);
            });

    connect(&m_playlistManager, &PlaylistManager::currentItemChanged,
            &m_skipManager, [this](const QModelIndex &index) {
                m_skipManager.onCurrentItemChanged(static_cast<PlaylistItem*>(index.internalPointer()));
            });

    connect(&m_playlistManager, &PlaylistManager::currentItemChanged,
            &m_discordPresence, [this](const QModelIndex &index) {
                m_discordPresence.onCurrentItemChanged(static_cast<PlaylistItem*>(index.internalPointer()));
            });

    // Keep the InfoPage's current-episode highlight in sync with live playback.
    connect(&m_playlistManager, &PlaylistManager::currentItemChanged,
            &m_showManager, [this](const QModelIndex &) { m_showManager.onPlaybackIndexChanged(); });

    connect(&m_libraryManager, &LibraryManager::fetchedAllEpCounts,
            &m_libraryProxyModel, &LibraryProxyModel::refreshFilter);

    connect(&m_showManager, &ShowManager::showChanged, this, [this]() {
        auto show = m_showManager.getShow();
        m_libraryManager.updateShowCover(show.link, show.coverUrl);
        auto playlist = m_showManager.getPlaylist();
        m_libraryManager.cacheHistoryMeta(show.link, show.title, show.coverUrl,
                                          show.provider ? show.provider->name() : QString(),
                                          playlist ? playlist->count() : 0);
        if (m_pendingAutoResume) {
            m_pendingAutoResume = false;
            continueWatching();
        }
    });

    std::setlocale(LC_NUMERIC, "C");
    QQuickStyle::setStyle("Universal");

    prefetchStartupData();
    checkForUpdates();
}

#ifndef APP_VERSION
#define APP_VERSION "0.0"
#endif

bool Application::isNewerVersion(const QString &latest, const QString &current) {
    const auto lp = latest.split('.');
    const auto cp = current.split('.');
    for (int i = 0; i < qMax(lp.size(), cp.size()); ++i) {
        int l = i < lp.size() ? lp[i].toInt() : 0;
        int c = i < cp.size() ? cp[i].toInt() : 0;
        if (l != c) return l > c;
    }
    return false;
}

void Application::checkForUpdates() {
    auto future = QtConcurrent::run([]() {
        Client client({}, false);
        auto resp = client.get("https://api.github.com/repos/jeffx13/AoNami/releases/latest",
                               {{"Accept", "application/vnd.github+json"}, {"User-Agent", "AoNami"}});
        auto obj = resp.toJsonObject();
        QString latest = obj.value("tag_name").toString();
        if (latest.startsWith('v') || latest.startsWith('V')) latest = latest.mid(1);
        if (latest.isEmpty() || !isNewerVersion(latest, QStringLiteral(APP_VERSION))) return;

        const QString url = obj.value("html_url").toString();
        QMetaObject::invokeMethod(&UiBridge::instance(), [latest, url]() {
            UiBridge::instance().showInfo(
                QStringLiteral("A new version (%1) is available.\n%2").arg(latest, url),
                QStringLiteral("Update Available"));
        }, Qt::QueuedConnection);
    });
    Q_UNUSED(future)
}

void Application::prefetchStartupData() {
    auto *provider = m_providerManager.getCurrentSearchProvider();
    if (!provider) return;
    int type = m_providerManager.getCurrentSearchType();
    m_searchManager.latest(1, type, provider);
}

Application::~Application() {
    m_migrateCancel.cancel();
    if (m_migrateFuture.isRunning()) {
        try { m_migrateFuture.waitForFinished(); }
        catch (...) { qWarning("Application: migrate threw during shutdown"); }
    }
    xmlCleanupParser();
}

void Application::setFont(const QString &fontPath) {
    int fontId = QFontDatabase::addApplicationFont(fontPath);
    auto families = QFontDatabase::applicationFontFamilies(fontId);
    if (fontId != -1 && !families.isEmpty())
        QGuiApplication::setFont(QFont(families.first(), 16));
}

void Application::explore(const QString &query, int page, bool isLatest) {
    auto *provider = m_providerManager.getCurrentSearchProvider();
    if (!provider) return;
    int type = m_providerManager.getCurrentSearchType();

    if (!query.isEmpty())    m_searchManager.search(query, page, type, provider);
    else if (isLatest)       m_searchManager.latest(page, type, provider);
    else                     m_searchManager.popular(page, type, provider);
}

void Application::loadResult(SearchManager &src, int index) {
    ShowData show = src.getResultAt(index);
    ShowData::LastWatchInfo info = m_libraryManager.getLastWatchInfo(show.link);
    info.playlist = m_playlistManager.find(show.link);
    m_showManager.setShow(show, info);
}

void Application::appendResult(SearchManager &src, int index, bool play) {
    auto show = src.getResultAt(index);
    if (!show.provider) return;
    ShowData::LastWatchInfo info = m_libraryManager.getLastWatchInfo(show.link);
    QSharedPointer<PlaylistItem> cached;
    if (m_showManager.getShow().link == show.link)
        cached = m_showManager.getPlaylist();
    m_playlistManager.appendShow(show.title, show.link, show.provider, cached, info, play);
}

// libraryType is passed in rather than derived: loadShow wants the displayed list, the resume
// paths want the entry's own.
void Application::openEntry(const QString &title, const QString &link, const QString &cover,
                            const QString &providerName, int libraryType,
                            int lastWatchedIndex, int timestamp, bool autoResume) {
    // Already the loaded show, so setShow would no-op - resume, or just show it.
    if (m_showManager.getShow().link == link) {
        m_pendingAutoResume = false;
        if (autoResume) continueWatching();
        else            UiBridge::instance().navigateTo(UiBridge::Page::Info);
        return;
    }

    auto *provider = m_providerManager.getProvider(providerName);
    if (!provider) {
        UiBridge::instance().showError(providerName + " does not exist", "Show Error");
        return;
    }
    ShowData show(title, link, cover, provider);
    ShowData::LastWatchInfo info;
    info.libraryType = libraryType;
    info.lastWatchedIndex = lastWatchedIndex;
    info.timestamp = timestamp;
    info.playlist = m_playlistManager.find(link);
    m_pendingAutoResume = autoResume;
    m_showManager.setShow(show, info);
}

// setShow would see the same link and short-circuit, so this goes straight to a load.
// The player's playlist is kept when it holds one, else the episode list is refetched.
void Application::reloadShow() {
    const ShowData current = m_showManager.getShow();
    if (current.link.isEmpty() || !current.provider) return;

    ShowData show(current.title, current.link, current.coverUrl, current.provider,
                  current.latestTxt, current.type);
    ShowData::LastWatchInfo info = m_libraryManager.getLastWatchInfo(current.link);
    info.playlist = m_playlistManager.find(current.link);
    m_showManager.reload(show, info);
}

void Application::loadShow(int index, bool fromLibrary) {
    m_pendingAutoResume = false;
    if (!fromLibrary) { loadResult(m_searchManager, index); return; }

    auto entry = m_libraryManager.getEntry(index);
    if (!entry.valid) return;
    openEntry(entry.title, entry.link, entry.cover, entry.provider,
              m_libraryManager.getDisplayLibraryType(),
              entry.lastWatchedIndex, entry.timestamp, false);
}

void Application::resumeFromLibrary(const QString &link) {
    auto entry = m_libraryManager.getEntryByLink(link);
    if (!entry.valid) return;
    openEntry(entry.title, entry.link, entry.cover, entry.provider, entry.libraryType,
              entry.lastWatchedIndex, entry.timestamp, true);
}

void Application::resumeFromHistory(const QString &link) {
    // Prefer the library entry (full state); fall back to the history table for non-library shows.
    QString title, cover, providerName;
    int libraryType = -1, lastWatchedIndex = -1, timestamp = 0;
    auto entry = m_libraryManager.getEntryByLink(link);
    if (entry.valid) {
        title = entry.title; cover = entry.cover; providerName = entry.provider;
        libraryType = entry.libraryType; lastWatchedIndex = entry.lastWatchedIndex; timestamp = entry.timestamp;
    } else {
        auto h = m_libraryManager.getHistoryEntry(link);
        if (!h.valid) return;
        title = h.title; cover = h.cover; providerName = h.provider;
        lastWatchedIndex = h.lastWatchedIndex; timestamp = h.timestamp;
    }
    openEntry(title, link, cover, providerName, libraryType, lastWatchedIndex, timestamp, true);
}

void Application::addToLibrary(int index, int libraryType) {
    auto show = (index == -1) ? m_showManager.getShow() : m_searchManager.getResultAt(index);
    m_libraryManager.add(show, libraryType);
}

void Application::searchOnProvider(const QString &providerName, const QString &query, int page) {
    ShowProvider *provider = m_providerManager.getProvider(providerName);
    if (!provider) {
        UiBridge::instance().showError(providerName + " does not exist", "Migrate");
        return;
    }
    m_migrateSearch.search(query, page, 0, provider);   // type 0 - providers don't share type indices
}

void Application::migrateShow(int libraryIndex, int resultIndex, int resumeEpisode) {
    const auto oldEntry = m_libraryManager.getEntry(libraryIndex);
    if (!oldEntry.valid) { UiBridge::instance().showError("Library item not found", "Migrate"); return; }
    if (resultIndex < 0 || resultIndex >= m_migrateSearch.count()) {
        UiBridge::instance().showError("No show selected", "Migrate"); return;
    }
    ShowData newShow = m_migrateSearch.getResultAt(resultIndex);
    if (!newShow.provider) { UiBridge::instance().showError("Selected show has no provider", "Migrate"); return; }

    const QString oldLink = oldEntry.link;
    // Only the episode being played can't be re-keyed underneath itself; a show merely open or
    // queued is fixed up once the new playlist is in hand.
    if (m_playlistManager.isPlaying(oldLink)) {
        UiBridge::instance().showError("This show is playing right now - stop it first.", "Migrate");
        return;
    }
    if (newShow.link != oldLink && m_libraryManager.linkExists(newShow.link)) {
        UiBridge::instance().showError("That show is already in your library.", "Migrate");
        return;
    }

    if (m_migrateFuture.isRunning()) {
        UiBridge::instance().showError("A migration is already running.", "Migrate");
        return;
    }

    const int timestamp = oldEntry.timestamp;
    ShowProvider *provider = newShow.provider;

    // last_watched_index is positional, so the episode has to be found by number in the new playlist.
    m_migrateCancel.reset();
    m_migrateFuture = QtConcurrent::run([this, newShow, oldLink, resumeEpisode, timestamp, provider,
                                         cancel = m_migrateCancel]() mutable {
        Client client(cancel);
        try {
            provider->getPlaylist(&client, newShow);
        } catch (const std::exception &e) {
            // Without this the future is discarded and Migrate just never finishes.
            const QString msg = QString::fromUtf8(e.what());
            QMetaObject::invokeMethod(&UiBridge::instance(), [msg]() {
                UiBridge::instance().showError("Could not load the show on that provider:\n" + msg, "Migrate");
            }, Qt::QueuedConnection);
            return;
        } catch (...) {
            oLog() << "Migrate" << "provider threw a non-standard exception";
            return;
        }
        if (cancel.isCancelled()) return;   // app closing - nothing to migrate into

        auto playlist = newShow.getPlaylist();
        const int total = playlist ? playlist->count() : 0;
        int targetIndex = qBound(0, resumeEpisode - 1, total > 0 ? total - 1 : 0);
        if (playlist) {
            for (int i = 0; i < playlist->count(); ++i) {
                auto ep = playlist->at(i);
                if (ep && int(ep->number) == resumeEpisode) { targetIndex = i; break; }
            }
        }
        const QString title = newShow.title, cover = newShow.coverUrl, newLink = newShow.link;
        const QString provName = provider->name();
        const int showType = newShow.type;
        QMetaObject::invokeMethod(this, [=, this]() {
            bool ok = m_libraryManager.migrate(oldLink, newLink, title, cover, provName, showType,
                                               targetIndex, timestamp, total);
            if (!ok) {
                UiBridge::instance().showError("Migration failed (target may already be in the library).", "Migrate");
                return;
            }
            // Same anime, so the skip times and MAL id still apply - move them to the new key.
            Settings &s = Settings::instance();
            const QString skipVal = s.getString(Config::skipProfile(oldLink));
            if (!skipVal.isEmpty()) s.setString(Config::skipProfile(newLink), skipVal);
            const QString malVal = s.getString(Config::skipMal(oldLink));
            if (!malVal.isEmpty()) s.setString(Config::skipMal(newLink), malVal);

            auto migrated = newShow.getPlaylist();
            if (migrated) migrated->setCurrentIndex(targetIndex);
            m_playlistManager.rekey(oldLink, migrated);
            if (newLink != oldLink && m_showManager.getShow().link == oldLink) {
                ShowData::LastWatchInfo info = m_libraryManager.getLastWatchInfo(newLink);
                info.playlist = migrated;
                m_showManager.setShow(newShow, info, false);
            }
            UiBridge::instance().showInfo("Migrated to " + provName + ".", "Migrate");
        }, Qt::QueuedConnection);
    });
}

void Application::playFromEpisodeList(int index, bool append) {
    auto playlist = m_showManager.getPlaylist();
    if (!playlist) return;
    playlist->setCurrentIndex(index);

    if (append) {
        m_playlistManager.append(playlist);
        return;
    }

    playlist->season = -1;
    auto first = m_playlistManager.root()->at(0);
    // Both return the *existing* row when this show is already queued, so playing row 0 blindly
    // would start whatever else is sitting at the top.
    const int row = (first && first->season == -1) ? m_playlistManager.replace(0, playlist)
                                                   : m_playlistManager.insert(0, playlist);
    if (row < 0) return;
    m_playlistManager.playPlaylist(row);
}

void Application::continueWatching() {
    playFromEpisodeList(qMax(m_showManager.getContinueIndex(), 0), false);
}

void Application::downloadCurrentShow(int startIndex, int endIndex) {
    if (endIndex < 0) endIndex = startIndex;
    m_downloadManager.downloadShow(m_showManager.getShow(), startIndex, endIndex);
}

void Application::appendToPlaylists(int index, bool fromLibrary, bool play) {
    if (!fromLibrary) { appendResult(m_searchManager, index, play); return; }

    auto entry = m_libraryManager.getEntry(index);
    if (!entry.valid) return;
    auto *provider = m_providerManager.getProvider(entry.provider);
    if (!provider) {
        UiBridge::instance().showError(entry.provider + " does not exist", "Show Error");
        return;
    }
    ShowData::LastWatchInfo info;
    info.lastWatchedIndex = entry.lastWatchedIndex;
    info.timestamp = entry.timestamp;

    QSharedPointer<PlaylistItem> cached;
    if (m_showManager.getShow().link == entry.link)
        cached = m_showManager.getPlaylist();

    m_playlistManager.appendShow(entry.title, entry.link, provider, cached, info, play);
}