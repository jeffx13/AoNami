#pragma once
#include <QDir>
#include <QStandardItemModel>
#include <QtConcurrent>
#include "core/showdata.h"
#include "player/serverlistmodel.h"
#include "player/subtitlelistmodel.h"
#include "player/videolistmodel.h"
#include "player/audiolistmodel.h"
#include "playlistitem.h"


class PlaylistManager : public QAbstractItemModel {
    Q_OBJECT
    Q_PROPERTY(QModelIndex currentModelIndex READ getCurrentModelIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(QModelIndex currentListIndex READ getCurrentListIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(ServerListModel *serverList READ getServerList CONSTANT)
    Q_PROPERTY(SubtitleListModel *subtitleList READ getSubtitleList CONSTANT)
    Q_PROPERTY(VideoListModel *videoList READ getVideoList CONSTANT)
    Q_PROPERTY(AudioListModel *audioList READ getAudioList CONSTANT)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)

public:
    explicit PlaylistManager(QObject *parent = nullptr);
    ~PlaylistManager() {
        //m_root->clear();
        delete m_root;
    }
    Q_SIGNAL void isLoadingChanged(void);
    Q_SIGNAL void currentIndexAboutToChange(void);
    Q_SIGNAL void currentIndexChanged(void);
    Q_SIGNAL void aboutToPlay(void);

    Q_INVOKABLE QModelIndex getCurrentIndex(QModelIndex i) const {
        auto currentPlaylist = static_cast<PlaylistItem *>(i.internalPointer());
        if (!currentPlaylist ||
            !currentPlaylist->isValidIndex(currentPlaylist->currentIndex))
            return QModelIndex();
        return index(currentPlaylist->currentIndex, 0, index(m_root->indexOf(currentPlaylist), 0, QModelIndex()));
    }

    int append(PlaylistItem *playlist);

    int insert(int index, PlaylistItem *playlist);
    int replace(int index, PlaylistItem *newPlaylist);
    Q_INVOKABLE void removeAt(int index);
    Q_INVOKABLE void clear();
    Q_INVOKABLE void setSubtitle(const QUrl &url);

    PlaylistItem *findPlaylist(const QString &link) {
        if (!playlistSet.contains(link)) return nullptr;
        return m_root->at(m_root->indexOf (link));
    }
    PlaylistItem *at(int index) const { return m_root->at(index); }
    PlaylistItem *getCurrentPlaylist() const { return m_root->getCurrentItem(); }

    Q_INVOKABLE void openUrl(QUrl url, bool playUrl);
    Q_INVOKABLE void loadServer(int index);
    Q_INVOKABLE void loadVideo(int index);
    Q_INVOKABLE void loadAudio(int index);
    Q_INVOKABLE void reload();

    QModelIndex getCurrentListIndex() {
        return createIndex(m_root->currentIndex, 0, m_root->getCurrentItem());
    }

    bool isLoading() { return m_isLoading; }

    //  Traversing the playlist
    Q_INVOKABLE bool tryPlay(int playlistIndex = -1, int itemIndex = -1);
    Q_INVOKABLE void loadOffset(int offset);
    Q_INVOKABLE void loadIndex(QModelIndex index);
    Q_INVOKABLE inline void playNextItem() { loadOffset(1); }
    Q_INVOKABLE inline void playPrecedingItem() { loadOffset(-1); }
    Q_INVOKABLE void cancel() {
        if (m_watcher.isRunning()) {
            m_isCancelled = true;
        }
    }
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
    SubtitleListModel m_subtitleListModel;
    VideoListModel m_videoListModel;
    AudioListModel m_audioListModel;
    VideoListModel *getVideoList() { return &m_videoListModel; }
    AudioListModel *getAudioList() { return &m_audioListModel; }
    ServerListModel *getServerList() { return &m_serverListModel; }
    SubtitleListModel *getSubtitleList() { return &m_subtitleListModel; }


    PlaylistItem *m_root = new PlaylistItem("root", nullptr, "/");
    QSet<QString> playlistSet; // Prevents the playlist with the same being added


    QModelIndex getCurrentModelIndex() const;


    bool registerPlaylist(PlaylistItem *playlist);
    void unregisterPlaylist(PlaylistItem *playlist);

    PlayItem play(int playlistIndex, int itemIndex);
    Q_SLOT void onLocalDirectoryChanged(const QString &path);

    void setCurrentPlayItem(const PlayItem &playItem) {
        m_currentPlayItem = playItem;
        m_subtitleListModel.setSubtitles(&m_currentPlayItem.subtitles);
        m_videoListModel.setVideos(&m_currentPlayItem.videos);
        m_audioListModel.setAudios(&m_currentPlayItem.audios);


    }

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

