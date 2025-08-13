#pragma once
#include <QDir>
#include <QFileSystemWatcher>
#include <QStandardItemModel>
#include "base/showdata.h"
#include "gui/serverlistmodel.h"
#include "playlistitem.h"
#include "base/servicemanager.h"


class PlaylistManager : public ServiceManager {
    Q_OBJECT
    Q_PROPERTY(ServerListModel *serverList READ getServerList CONSTANT)
public:
    explicit PlaylistManager(QObject *parent = nullptr);
    ~PlaylistManager() { delete m_root; }

    PlaylistItem *root() const { return m_root; }
    PlaylistItem *find(const QString &link);
    int count() const { return m_root->count(); }
    int append(PlaylistItem *playlist);
    int insert(int index, PlaylistItem *playlist);
    int replace(int index, PlaylistItem *playlist);
    Q_INVOKABLE void remove(QModelIndex index);
    Q_INVOKABLE void clear();

    //  Traversing the playlist
    Q_INVOKABLE bool tryPlay(int playlistIndex = -1, int itemIndex = -1);
    Q_INVOKABLE void loadNextItem(int offset = 1);
    Q_INVOKABLE void loadNextPlaylist(int offset = 1);
    Q_INVOKABLE void loadIndex(const QModelIndex &index);
    Q_INVOKABLE void reload();
    Q_INVOKABLE void openUrl(QUrl url, bool play);
    Q_INVOKABLE void loadServer(int index);

    Q_INVOKABLE void showCurrentItemName() const;
    Q_INVOKABLE void saveProgress() const;

    void updateSelection(bool scrollToIndex = false);
    Q_INVOKABLE void cancel();

    // Signals
    Q_SIGNAL void aboutToInsert(PlaylistItem *parent, int index);
    Q_SIGNAL void inserted();
    Q_SIGNAL void aboutToRemove(QModelIndex modelIndex);
    Q_SIGNAL void removed();
    Q_SIGNAL void changed(int playlistIndex);
    Q_SIGNAL void currentIndexChanged(int playlistIndex, int itemIndex, bool scrollToIndex);
    Q_SIGNAL void modelReset();

    Q_SIGNAL void progressUpdated(QString link, int progressIndex, int timestamp) const;
    Q_SIGNAL void aboutToPlay(void) const;
private:
    PlaylistItem *m_root = new PlaylistItem("root", nullptr, "/");
    Client m_client = Client(&m_isCancelled);
    PlayItem play(int playlistIndex, int itemIndex);

    // Watchers
    QFileSystemWatcher m_folderWatcher;
    QFutureWatcher<PlayItem> m_watcher;

    // models
    ServerListModel m_serverListModel;
    ServerListModel *getServerList() { return &m_serverListModel; }

    // Prevents the playlist with the same link being added
    QSet<QString> m_playlistSet;
    bool registerPlaylist(PlaylistItem *playlist);
    void deregisterPlaylist(PlaylistItem *playlist);

    PlaylistItem *loadFromFolder(const QUrl &pathUrl, PlaylistItem *playlist = nullptr);

    Q_SLOT void onLocalDirectoryChanged(const QString &path);
    Q_SLOT void onLoadFinished();
private:
    PlayItem m_currentPlayItem;
};

