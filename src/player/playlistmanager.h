#pragma once
#include <QDir>
#include <QFileSystemWatcher>
#include <QStandardItemModel>
#include "core/showdata.h"
#include "player/serverlistmodel.h"
#include "player/tracklistmodel.h"
#include "playlistitem.h"


class PlaylistManager : public QAbstractItemModel {
    Q_OBJECT
    Q_PROPERTY(QModelIndex currentModelIndex READ getCurrentModelIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(QModelIndex currentListIndex READ getCurrentListIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(ServerListModel *serverList READ getServerList CONSTANT)
    Q_PROPERTY(TrackListModel *subtitleList READ getSubtitleList CONSTANT)
    Q_PROPERTY(TrackListModel *videoList READ getVideoList CONSTANT)
    Q_PROPERTY(TrackListModel *audioList READ getAudioList CONSTANT)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)

public:
    explicit PlaylistManager(QObject *parent = nullptr);
    ~PlaylistManager() {
        //m_root->clear();
        delete m_root;
    }

    Q_SIGNAL void isLoadingChanged(void);
    Q_SIGNAL void updateTimeStamp(void);
    Q_SIGNAL void currentIndexChanged(void);
    Q_SIGNAL void aboutToPlay(void);

    int append(PlaylistItem *playlist);
    int insert(int index, PlaylistItem *playlist);
    int replace(int index, PlaylistItem *newPlaylist);
    Q_INVOKABLE void removeByModelIndex(QModelIndex index) {
        auto playlist = ((PlaylistItem*)(index.constInternalPointer()));
        if (playlist->parent() != m_root) return;
        int i = m_root->indexOf(playlist);
        removeAt(i);
    }
    Q_INVOKABLE void removeAt(int index);
    Q_INVOKABLE void clear();
    PlaylistItem *at(int index) const { return m_root->at(index); }
    PlaylistItem *getCurrentPlaylist() const { return m_root->getCurrentItem(); }
    QModelIndex getCurrentListIndex();
    Q_INVOKABLE QModelIndex getCurrentIndex(QModelIndex i) const;
    PlaylistItem *find(const QString &link) {
        if (!playlistSet.contains(link)) return nullptr;
        return m_root->at(m_root->indexOf (link));
    }

    Q_INVOKABLE void openUrl(QUrl url, bool playUrl);
    Q_INVOKABLE void loadServer(int index);
    Q_INVOKABLE void loadVideo(int index);
    Q_INVOKABLE void loadAudio(int index);
    Q_INVOKABLE void loadSubtitle(int index);
    Q_INVOKABLE void reload();
    int count() const { return m_root->size(); }


    //  Traversing the playlist
    Q_INVOKABLE bool tryPlay(int playlistIndex = -1, int itemIndex = -1);
    Q_INVOKABLE void loadOffset(int offset);
    Q_INVOKABLE void loadIndex(QModelIndex index);
    Q_INVOKABLE void cancel() {
        if (m_watcher.isRunning()) {
            m_isCancelled = true;
        }
    }
    bool isLoading() { return m_isLoading; }
    Q_INVOKABLE void showCurrentItemName() const;

private:
    bool m_isLoading = false;
    void setIsLoading(bool value);
    std::atomic<bool> m_isCancelled = false;
    Client m_client = Client(&m_isCancelled);

    // watchers
    QFileSystemWatcher m_folderWatcher;
    QFutureWatcher<PlayItem> m_watcher;

    // models
    ServerListModel m_serverListModel;




    ServerListModel *getServerList() { return &m_serverListModel; }
    TrackListModel *getSubtitleList();
    TrackListModel *getVideoList();
    TrackListModel *getAudioList();




    PlaylistItem *m_root = new PlaylistItem("root", nullptr, "/");
    QSet<QString> playlistSet; // Prevents the playlist with the same being added


    QModelIndex getCurrentModelIndex() const;


    bool registerPlaylist(PlaylistItem *playlist);
    void unregisterPlaylist(PlaylistItem *playlist);

    PlayItem play(int playlistIndex, int itemIndex);
    Q_SLOT void onLocalDirectoryChanged(const QString &path);

    //bool parseLocalVideo(PlayItem &playItem);

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
    };
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &childIndex) const override;
    int columnCount(const QModelIndex &parent) const override { return 1; }
};

