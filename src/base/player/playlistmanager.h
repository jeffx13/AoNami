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

    Q_SIGNAL void removed(QModelIndex modelIndex);
    Q_SIGNAL void changed(int index);
    Q_SIGNAL void currentIndexChanged(int playlistIndex, int itemIndex, bool scrollToIndex);
    Q_SIGNAL void modelReset();
    Q_SIGNAL void inserted(PlaylistItem *parent, int index);


    Q_SIGNAL void progressUpdated(QString link, int progressIndex, int timestamp) const;
    Q_SIGNAL void aboutToPlay(void) const;
    Q_INVOKABLE void saveProgress() const;



    void updateSelection(bool scrollToIndex = false) {
        int playlistIndex = m_root->getCurrentIndex();
        int itemIndex = playlistIndex != -1 ? m_root->getCurrentItem()->getCurrentIndex() : -1;
        emit currentIndexChanged(playlistIndex, itemIndex, scrollToIndex);
    }

    PlaylistItem *root() const { return m_root; }
    int append(PlaylistItem *playlist);
    int insert(int index, PlaylistItem *playlist);
    int replace(int index, PlaylistItem *playlist);
    Q_INVOKABLE void removeByModelIndex(QModelIndex index);
    Q_INVOKABLE void clear();

    PlaylistItem *at(int index) const { return m_root->at(index); }

    PlaylistItem *find(const QString &link) {
        if (!playlistSet.contains(link)) return nullptr;
        return m_root->at(m_root->indexOf(link));
    }


    int count() const { return m_root->size(); }

    //  Traversing the playlist
    Q_INVOKABLE bool tryPlay(int playlistIndex = -1, int itemIndex = -1);
    Q_INVOKABLE void loadNextItem(int offset = 1);
    Q_INVOKABLE void loadNextPlaylist(int offset = 1);
    Q_INVOKABLE void loadIndex(QModelIndex index);

    Q_INVOKABLE void openUrl(QUrl url, bool playUrl);
    Q_INVOKABLE void loadServer(int index);

    Q_INVOKABLE void reload();

    Q_INVOKABLE void cancel() {
        if (m_watcher.isRunning()) {
            m_isCancelled = true;
        }
    }

    Q_INVOKABLE void showCurrentItemName() const;



private:
    Client m_client = Client(&m_isCancelled);

    // Watchers
    QFileSystemWatcher m_folderWatcher;
    QFutureWatcher<PlayItem> m_watcher;

    // models
    ServerListModel m_serverListModel;
    ServerListModel *getServerList() { return &m_serverListModel; }

    PlaylistItem *m_root = new PlaylistItem("root", nullptr, "/");

    // Prevents the playlist with the same link being added
    QSet<QString> playlistSet;
    bool registerPlaylist(PlaylistItem *playlist);
    void deregisterPlaylist(PlaylistItem *playlist);


    PlayItem play(int playlistIndex, int itemIndex);
    PlaylistItem *loadFromFolder(const QUrl &pathUrl, PlaylistItem *playlist = nullptr);

    Q_SLOT void onLocalDirectoryChanged(const QString &path);
    Q_SLOT void onLoadFinished();
private:
    PlayItem m_currentPlayItem;
};

