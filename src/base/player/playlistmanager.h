#pragma once
#include <QDir>
#include <QFileSystemWatcher>
#include <QStandardItemModel>
#include "base/showdata.h"
#include "gui/models/serverlistmodel.h"
#include "playlistitem.h"
#include "base/servicemanager.h"

class PlaylistManager : public ServiceManager {
    Q_OBJECT
    Q_PROPERTY(ServerListModel *serverList READ getServerList CONSTANT)
public:
    explicit PlaylistManager(QObject *parent = nullptr);
    ~PlaylistManager() { }

    PlaylistItem *root() const { return m_root.get(); }
    PlaylistItem *find(const QString &link) {
        return m_playlistMap.value(link, nullptr);
    }
    int count() const { return m_root->count(); }

    int append(PlaylistItem *playlist, PlaylistItem *parent = nullptr) { return insert(INT_MAX, playlist, parent); }
    int insert(int index, PlaylistItem *playlist, PlaylistItem *parent = nullptr);
    int replace(int index, PlaylistItem *playlist, PlaylistItem *parent = nullptr);
    Q_INVOKABLE void remove(QModelIndex index);
    Q_INVOKABLE void clear();

    //  Traversing the playlist
    Q_INVOKABLE bool tryPlay(int playlistIndex = -1, int itemIndex = -1);
    bool tryPlay(PlaylistItem *item);

    Q_INVOKABLE void loadNextItem(int offset = 1);
    Q_INVOKABLE void loadNextPlaylist(int offset = 1);
    Q_INVOKABLE void loadIndex(const QModelIndex &index) { tryPlay(static_cast<PlaylistItem *>(index.internalPointer())); }
    Q_INVOKABLE void reload();
    Q_INVOKABLE void openUrl(QUrl url, bool play);
    Q_INVOKABLE void loadServer(int index);

    Q_INVOKABLE void showCurrentItemName() const;
    Q_INVOKABLE void saveProgress() const;
    Q_INVOKABLE void cancel();

    // Signals
    Q_SIGNAL void aboutToInsert(PlaylistItem *parent, int index);
    Q_SIGNAL void inserted();
    Q_SIGNAL void aboutToRemove(PlaylistItem *item);
    Q_SIGNAL void removed();
    Q_SIGNAL void dataChanged(PlaylistItem *item);
    Q_SIGNAL void updateSelections(PlaylistItem *currentItem, bool scrollToIndex = false); //int playlistIndex, int itemIndex,
    Q_SIGNAL void modelReset();
    Q_SIGNAL void progressUpdated(QString link, int progressIndex, int timestamp) const;

private:
    std::unique_ptr<PlaylistItem> m_root = std::unique_ptr<PlaylistItem>(new PlaylistItem("root", nullptr, "/"));
    Client m_client = Client(&m_isCancelled);

    PlayInfo play(PlaylistItem *item);

    PlaylistItem *m_currentItem = nullptr;
    void setCurrentItem(PlaylistItem *currentItem);

    // Watchers
    QFileSystemWatcher m_folderWatcher;
    QFutureWatcher<PlayInfo> m_watcher;

    // models
    ServerListModel m_serverListModel;
    ServerListModel *getServerList() { return &m_serverListModel; }

    // Prevents the playlist with the same link being added
    QMap<QString, PlaylistItem*> m_playlistMap;
    void registerPlaylist(PlaylistItem *playlist);
    void deregisterPlaylist(PlaylistItem *playlist);
    QStringList m_playableExtensions {"*.mp4", "*.mkv", "*.avi", "*.mp3", "*.flac", "*.wav", "*.ogg", "*.webm", "*.m3u8", "*.mov"};

    bool loadFromFolder(const QUrl &pathUrl, PlaylistItem *playlist, bool recursive = true);

    Q_SLOT void onLocalDirectoryChanged(const QString &path);
    Q_SLOT void onLoadFinished();
private:
    PlayInfo m_currentPlayItem;
};

