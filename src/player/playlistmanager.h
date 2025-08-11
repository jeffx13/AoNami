#pragma once
#include <QDir>
#include <QFileSystemWatcher>
#include <QStandardItemModel>
#include "core/showdata.h"
#include "player/serverlistmodel.h"
#include "playlistitem.h"


class PlaylistManager : public QAbstractItemModel {
    Q_OBJECT
    // Q_PROPERTY(QModelIndex currentModelIndex READ getCurrentModelIndex NOTIFY currentIndexChanged)
    // Q_PROPERTY(QModelIndex currentListIndex READ getCurrentListIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(ServerListModel *serverList READ getServerList CONSTANT)

    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)

public:
    explicit PlaylistManager(QObject *parent = nullptr);
    ~PlaylistManager() {
        //m_root->clear();
        delete m_root;
    }

    Q_SIGNAL void isLoadingChanged(void);
    Q_SIGNAL void updateTimeStamp(void);
    // Q_SIGNAL void currentIndexChanged(QModelIndex);
    Q_SIGNAL void selectionsChanged(QModelIndex, bool);
    Q_SIGNAL void aboutToPlay(void);

    void updateSelection(bool scrollToIndex = false) {
        int playlistIndex = m_root->currentIndex;
        if (playlistIndex == -1) {
            emit selectionsChanged(QModelIndex(), false);
            return;
        }
        int itemIndex = m_root->getCurrentItem()->currentIndex;
        if (itemIndex == -1) {
            emit selectionsChanged(QModelIndex(), false);
            return;
        }

        auto parentIndex = index(playlistIndex, 0, QModelIndex());
        auto childIndex = index(itemIndex, 0, parentIndex);
        emit selectionsChanged(childIndex, scrollToIndex);

    }

    int append(PlaylistItem *playlist);
    int insert(int index, PlaylistItem *playlist);
    int replace(int index, PlaylistItem *newPlaylist);
    // void removeAt(int index);
    Q_INVOKABLE void removeByModelIndex(QModelIndex index);
    Q_INVOKABLE void clear();

    PlaylistItem *at(int index) const { return m_root->at(index); }
    PlaylistItem *getCurrentPlaylist() const { return m_root->getCurrentItem(); }
    QModelIndex getCurrentListIndex();
    Q_INVOKABLE QModelIndex getCurrentIndex(QModelIndex i) const;
    PlaylistItem *find(const QString &link) {
        if (!playlistSet.contains(link)) return nullptr;
        return m_root->at(m_root->indexOf (link));
    }


    int count() const { return m_root->size(); }

    //  Traversing the playlist
    Q_INVOKABLE bool tryPlay(int playlistIndex = -1, int itemIndex = -1);
    Q_INVOKABLE void loadNextItem(int offset = 1) {
        auto playlistIndex = m_root->getCurrentIndex();
        if (playlistIndex == -1) {
            tryPlay(0);
            return;
        }
        auto playlist = m_root->getCurrentItem();
        auto itemIndex = playlist->getCurrentIndex() + offset; // Impossible for current item index to be -1

        if (itemIndex == playlist->size() && playlistIndex + 1 < m_root->size()) {
            // Play next playlist
            playlistIndex += 1;
            itemIndex = m_root->at(playlistIndex)->getCurrentIndex();
        } else if (itemIndex == -1 && playlistIndex - 1 >= 0) {
            // Play previous playlist
            playlistIndex -= 1;
            itemIndex = m_root->at(playlistIndex)->getCurrentIndex();
        } else if (!playlist->isValidIndex(itemIndex)) {
            return;
        }

        tryPlay(playlistIndex, itemIndex);

    }


    Q_INVOKABLE void loadNextPlaylist(int offset = 1) {
        auto playlistIndex = m_root->getCurrentIndex();
        if (playlistIndex == -1) {
            tryPlay(0);
            return;
        }
        int nextPlaylistIndex = playlistIndex + offset;
        if (!m_root->isValidIndex(nextPlaylistIndex))
            return;

        tryPlay(nextPlaylistIndex);
    }

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
    bool m_isLoading = false;

    void setIsLoading(bool value);
    std::atomic<bool> m_isCancelled = false;
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

    QModelIndex getCurrentModelIndex() const;

    PlayItem play(int playlistIndex, int itemIndex);
    Q_SLOT void onLocalDirectoryChanged(const QString &path);

    Q_SLOT void onLoadFinished();

private:

    PlayItem m_currentPlayItem;
    enum
    {
        TitleRole = Qt::UserRole,
        IndexRole,
        NumberRole,
        NumberTitleRole,
        IsCurrentIndexRole,
        IsDeletableRole
    };
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &childIndex) const override;
    int columnCount(const QModelIndex &parent) const override { return 1; }
};

