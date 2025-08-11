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
    // Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)

public:
    explicit PlaylistManager(QObject *parent = nullptr);
    ~PlaylistManager() {
        delete m_root;
    }

    Q_SIGNAL void updateTimeStamp(void);
    Q_SIGNAL void aboutToPlay(void);


    Q_SIGNAL void removed(QModelIndex modelIndex);
    Q_SIGNAL void changed(int index);
    Q_SIGNAL void updateSelections(int playlistIndex, int itemIndex, bool scrollToIndex);
    Q_SIGNAL void modelReset();
    Q_SIGNAL void inserted(PlaylistItem *parent, int index);

    PlaylistItem *root() const { return m_root; }

    void updateSelection(bool scrollToIndex = false) {
        int playlistIndex = m_root->currentIndex;
        // if (playlistIndex == -1) {
        //     emit selectionsChanged(QModelIndex(), false);
        //     return;
        // }
        int itemIndex = playlistIndex != -1 ? m_root->getCurrentItem()->currentIndex : -1;
        // if (itemIndex == -1) {
        //     emit selectionsChanged(QModelIndex(), false);
        //     return;
        // }
        emit updateSelections(playlistIndex, itemIndex, scrollToIndex);


    }



    int append(PlaylistItem *playlist);
    int insert(int index, PlaylistItem *playlist);
    int replace(int index, PlaylistItem *newPlaylist);
    Q_INVOKABLE void removeByModelIndex(QModelIndex index);
    Q_INVOKABLE void clear();

    PlaylistItem *at(int index) const { return m_root->at(index); }
    PlaylistItem *getCurrentPlaylist() const { return m_root->getCurrentItem(); }
    // QModelIndex getCurrentListIndex();

    PlaylistItem *find(const QString &link) {
        if (!playlistSet.contains(link)) return nullptr;
        return m_root->at(m_root->indexOf (link));
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

    PlaylistItem *loadFromFolder(const QUrl &pathUrl, PlaylistItem *playlist = nullptr);

    bool isLoading() { return m_isLoading; }

signals:
    void indexAboutToChange();

private:
    // bool m_isLoading = false;
    // void setIsLoading(bool value);
    // std::atomic<bool> m_isCancelled = false;

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
    void unregisterPlaylist(PlaylistItem *playlist);

    // QModelIndex getCurrentModelIndex() const;

    PlayItem play(int playlistIndex, int itemIndex);
    Q_SLOT void onLocalDirectoryChanged(const QString &path);

    Q_SLOT void onLoadFinished();

private:

    PlayItem m_currentPlayItem;
    // enum
    // {
    //     TitleRole = Qt::UserRole,
    //     IndexRole,
    //     NumberRole,
    //     NumberTitleRole,
    //     IsCurrentIndexRole,
    //     IsDeletableRole
    // };
    // int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    // QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    // QHash<int, QByteArray> roleNames() const override;
    // QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    // QModelIndex parent(const QModelIndex &childIndex) const override;
    // int columnCount(const QModelIndex &parent) const override { return 1; }
};

