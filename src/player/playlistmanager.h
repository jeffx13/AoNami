#pragma once
#include <QDir>
#include <QStandardItemModel>
#include <QtConcurrent>
#include "core/showdata.h"
#include "player/serverlistmodel.h"
#include "player/subtitlelistmodel.h"
#include "playlistitem.h"
class PlaylistManager : public QAbstractItemModel {
    Q_OBJECT
    Q_PROPERTY(QModelIndex currentModelIndex READ getCurrentModelIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(QModelIndex currentListIndex READ getCurrentListIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(QString currentItemName READ getCurrentItemName NOTIFY currentIndexChanged)
    Q_PROPERTY(ServerListModel *serverList READ getServerList CONSTANT)
    Q_PROPERTY(SubtitleListModel *subtitleList READ getSubtitleList CONSTANT)
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
private:
    bool m_isLoading = false;
    void setIsLoading(bool value);
    std::atomic<bool> m_isCancelled = false;
    Client m_client = Client(&m_isCancelled);

    // watchers
    QFileSystemWatcher m_folderWatcher;
    QFutureWatcher<PlayInfo> m_watcher;

    // models
    ServerListModel m_serverListModel;
    SubtitleListModel m_subtitleListModel;
    ServerListModel *getServerList() { return &m_serverListModel; }
    SubtitleListModel *getSubtitleList() { return &m_subtitleListModel; }


    PlaylistItem *m_root = new PlaylistItem("root", nullptr, "/");
    QSet<QString> playlistSet; // Prevents the playlist with the same being added

    QString getCurrentItemName() const;
    QModelIndex getCurrentModelIndex() const;


    bool registerPlaylist(PlaylistItem *playlist);
    void unregisterPlaylist(PlaylistItem *playlist);

    PlayInfo play(int playlistIndex, int itemIndex);
    Q_SLOT void onLocalDirectoryChanged(const QString &path);


private:
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

