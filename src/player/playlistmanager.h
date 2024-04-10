#pragma once
#include <QDir>
#include <QStandardItemModel>
#include <QtConcurrent>
#include "data/showdata.h"
#include "player/serverlistmodel.h"
#include "data/playlistitem.h"

// #include <QAbstractItemModel>

class PlaylistManager : public QAbstractItemModel {
    Q_OBJECT
    Q_PROPERTY(QModelIndex currentIndex READ getCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(QModelIndex currentListIndex READ getCurrentListIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(QString currentItemName READ getCurrentItemName NOTIFY currentIndexChanged)
    Q_PROPERTY(ServerListModel *serverList READ getServerList CONSTANT)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)

private:
    bool m_isLoading = false;
    bool isLoading() { return m_isLoading; }
    void setIsLoading(bool value);

    QFileSystemWatcher m_folderWatcher;
    QFutureWatcher<void> m_watcher;
    ServerListModel m_serverList;
    PlaylistItem *m_root = new PlaylistItem("root", nullptr, "/");
    QSet<QString> playlistSet; // Prevents the playlist with the same being added

    ServerListModel *getServerList() { return &m_serverList; }
    QString getCurrentItemName() const;
    QModelIndex getCurrentIndex() const;

    bool registerPlaylist(PlaylistItem *playlist);
    void deregisterPlaylist(PlaylistItem *playlist);

    void play(int playlistIndex, int itemIndex);
    Q_SLOT void onLocalDirectoryChanged(const QString &path);
public:
    explicit PlaylistManager(QObject *parent = nullptr);

    ~PlaylistManager() { delete m_root; }
    void appendPlaylist(PlaylistItem *playlist);
    // void replaceCurrentPlaylist(PlaylistItem *playlist);

    void replaceMainPlaylist(PlaylistItem *playlist) {
        // Main playlist is the first playlist
        if (m_root->isEmpty()) {
            appendPlaylist (playlist); // root is empty so we append the playlist
        }
        else if (m_root->at (0)->link != playlist->link) {
            replacePlaylistAt (0, playlist);
        }
    }

    void replacePlaylistAt(int index, PlaylistItem *newPlaylist) {
        if (!newPlaylist || !m_root->isValidIndex(index)) return;

        auto playlistToReplace = m_root->at(index);
        registerPlaylist(newPlaylist);
        deregisterPlaylist(playlistToReplace);

        beginRemoveRows(QModelIndex(), index, index);
        endRemoveRows();
        beginInsertRows(QModelIndex(), index, index);
        m_root->replace(index, newPlaylist);
        endInsertRows();

        qDebug() << "Log (Playlist)   : Replaced index" << index
                 << "with" << newPlaylist->link;
    }


    PlaylistItem *getCurrentPlaylist() const { return m_root->getCurrentItem(); }

    Q_INVOKABLE void openUrl(const QUrl &url, bool playUrl);
    Q_INVOKABLE void loadIndex(QModelIndex index);
    Q_INVOKABLE void pasteOpen();

    // Q_INVOKABLE bool isList(QModelIndex index) {
    //     auto item = static_cast<const PlaylistItem*>(index.constInternalPointer());
    //     return item->type == PlaylistItem::LIST;
    // }
    QModelIndex getCurrentListIndex() {
        return createIndex (m_root->currentIndex, 0, m_root->getCurrentItem());

    }


    //  Traversing the playlist
    Q_INVOKABLE bool tryPlay(int playlistIndex = -1, int itemIndex = -1);
    Q_INVOKABLE void loadOffset(int offset);
    Q_INVOKABLE inline void playNextItem() { loadOffset(1); }
    Q_INVOKABLE inline void playPrecedingItem() { loadOffset(-1); }

    Q_SIGNAL void isLoadingChanged(void);
    Q_SIGNAL void currentIndexChanged(void);
    Q_SIGNAL void aboutToPlay(void);


private:
    enum
    {
        TitleRole = Qt::UserRole,
        IndexRole,
        NumberRole,
        NumberTitleRole
    };
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &childIndex) const override;
    int columnCount(const QModelIndex &parent) const override { return 1; }
};

