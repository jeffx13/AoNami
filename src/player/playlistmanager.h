#pragma once
#include <QFileSystemWatcher>
#include <QFuture>
#include <QHash>
#include <QSet>
#include <QSharedPointer>
#include <QTimer>
#include <QWeakPointer>
#include "core/showdata.h"
#include "ui/models/serverlistmodel.h"
#include "playlistitem.h"
#include "core/listmodel.h"

class ShowProvider;

class PlaylistManager : public TreeModel {
    Q_OBJECT
    Q_PROPERTY(ServerListModel *serverList READ getServerList CONSTANT)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)
public:
    explicit PlaylistManager(QObject *parent = nullptr);
    ~PlaylistManager();

    QSharedPointer<PlaylistItem> root() const { return m_root; }
    QSharedPointer<PlaylistItem> find(const QString &link);
    int count() const { return m_root->count(); }

    int append(const QSharedPointer<PlaylistItem> &playlist, const QSharedPointer<PlaylistItem> &parent = nullptr);
    int insert(int index, const QSharedPointer<PlaylistItem> &playlist, const QSharedPointer<PlaylistItem> &parent = nullptr);
    int replace(int index, const QSharedPointer<PlaylistItem> &playlist, const QSharedPointer<PlaylistItem> &parent = nullptr);
    Q_INVOKABLE void remove(const QModelIndex &index);
    Q_INVOKABLE void clear();

    Q_INVOKABLE bool playPlaylist(int index);
    Q_INVOKABLE void loadNextItem(int offset = 1);
    Q_INVOKABLE void loadNextPlaylist(int offset = 1);
    Q_INVOKABLE void loadIndex(const QModelIndex &index);
    Q_INVOKABLE void reload();
    Q_INVOKABLE void loadServer(int index);
    Q_INVOKABLE void tryNextServer();   // auto-fallback when the current server fails to play
    Q_INVOKABLE void showCurrentItemName() const;
    Q_INVOKABLE void saveProgress() const;
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void openUrl(QUrl url, bool play);

    // Append a show's playlist to the tree, fetching async if not cached.
    void appendShow(const QString &title, const QString &link, ShowProvider *provider,
                    QSharedPointer<PlaylistItem> cached, const ShowData::LastWatchInfo &info, bool play);

    enum { TitleRole = Qt::UserRole, IndexRole, NumberRole, IsCurrentIndexRole, IsDeletableRole, LinkRole, IsWatchedRole };
    Q_INVOKABLE QModelIndex getCurrentIndex(const QModelIndex &idx) const;
    Q_INVOKABLE QString currentShowName() const;        // name of the playing show/playlist
    Q_INVOKABLE QString currentItemName() const;        // display name of the playing episode
    Q_INVOKABLE int     currentShowEpisodeCount() const; // episodes in the playing playlist
    // True for a leaf episode hidden by the sidebar filter (collapsed to 0 height).
    Q_INVOKABLE bool isFilteredOut(const QModelIndex &index, const QString &filter) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &childIndex) const override;
    int columnCount(const QModelIndex &parent) const override { return 1; }

    // True while a play resolves (incl. the pending-restart window) - avoids spinner blink.
    bool isLoading() const { return m_watcher.isRunning() || m_pendingItem || m_pendingServerIndex >= 0; }

    Q_SIGNAL void currentItemChanged(const QModelIndex &index);
    Q_SIGNAL void isLoadingChanged();
    Q_SIGNAL void progressUpdated(QString link, int progressIndex, int timestamp, bool completed) const;
    Q_SIGNAL void episodeStarted(QString link, int index, int timestamp) const;   // -> history

private:
    QSharedPointer<PlaylistItem> m_root = QSharedPointer<PlaylistItem>::create("root", nullptr, "/");
    QWeakPointer<PlaylistItem> m_currentItem;
    QHash<QString, QWeakPointer<PlaylistItem>> m_playlistMap;
    ServerListModel m_serverListModel;
    QFileSystemWatcher m_folderWatcher;

    QFutureWatcher<PlayInfo> m_watcher;
    QSharedPointer<PlaylistItem> m_pendingItem;
    int m_pendingServerIndex = -1;
    QSet<QString> m_autoTriedServers;   // servers auto-fallback already tried this episode

    CancelToken       m_appendCancel;
    QFuture<void>     m_appendFuture;

    CancelToken       m_bgCacheCancel;
    QFuture<void>     m_bgCacheFuture;
    void cacheRemainingServers();

    // Flip the episode complete/incomplete the moment it crosses the watched threshold.
    bool m_currentCompleted     = false;
    bool m_mpvProgressConnected = false;
    void ensureMpvProgressConnection();
    void onPlaybackProgress();

    // Next-episode prefetch: resolve the next episode's working server in the
    // background while the current one plays, so advancing is near-instant.
    struct Prefetch {
        bool valid = false;
        QString itemLink;
        QList<VideoServer> servers;
        ShowProvider *provider = nullptr;
        int chosenIndex = -1;
        QHash<QString, PlayInfo> cachedSources;
        PlayInfo playInfo;
    };
    Prefetch          m_prefetch;
    CancelToken       m_prefetchCancel;
    QFuture<void>     m_prefetchFuture;
    QTimer            m_prefetchTimer;   // debounces prefetch so the current episode buffers first
    void prefetchNextEpisode();          // (re)arm the prefetch for the current episode's successor
    void startNextEpisodePrefetch();     // timer slot: launch the background resolve
    QSharedPointer<PlaylistItem> computeNextItem() const;
    bool tryUsePrefetch(const QSharedPointer<PlaylistItem> &item);
    void applyServerResult(const QList<VideoServer> &servers, ShowProvider *provider,
                           int chosenIndex, QHash<QString, PlayInfo> cache);

    void onPlayFinished();
    bool tryPlay(const QSharedPointer<PlaylistItem> &item);
    PlayInfo play(const QSharedPointer<PlaylistItem> &item);
    QSharedPointer<PlaylistItem> resolveToPlayableItem(QSharedPointer<PlaylistItem> item);
    PlayInfo loadPlayInfo(const QSharedPointer<PlaylistItem> &item);
    PlayInfo loadPastedPlayInfo(const QSharedPointer<PlaylistItem> &item);
    PlayInfo loadOnlinePlayInfo(const QSharedPointer<PlaylistItem> &item);
    PlayInfo loadLocalPlayInfo(const QSharedPointer<PlaylistItem> &item);
    void finalizePlayback(const QSharedPointer<PlaylistItem> &item);
    void setCurrentItem(const QSharedPointer<PlaylistItem> &currentItem);

    QModelIndex indexFor(PlaylistItem *item) const {
        return (!item || item == m_root.data()) ? QModelIndex() : createIndex(item->row(), 0, item);
    }

    void openLocalPath(const QUrl &url, const QString &urlString, bool play);
    void openRemoteUrl(const QString &urlString, const QUrl &url, bool play);

    Q_SLOT void onLocalDirectoryChanged(const QString &path);

    void registerPlaylist(const QSharedPointer<PlaylistItem> &playlist);
    void deregisterPlaylist(const QSharedPointer<PlaylistItem> &playlist);
    using PlaylistVisitor = std::function<void(const QSharedPointer<PlaylistItem> &)>;
    void visitListNodes(const QSharedPointer<PlaylistItem> &root, const PlaylistVisitor &visitor);
    ServerListModel *getServerList() { return &m_serverListModel; }
};