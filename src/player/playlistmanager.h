#pragma once
#include <QDir>
#include <QStandardItemModel>
#include <QtConcurrent>
#include "core/showdata.h"
#include "player/serverlistmodel.h"
#include "player/subtitlelistmodel.h"
#include "playlistitem.h"

// #include <QAbstractItemModel>

class PlaylistManager : public QAbstractItemModel {
    Q_OBJECT
    Q_PROPERTY(QModelIndex currentIndex READ getCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(QModelIndex currentListIndex READ getCurrentListIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(QString currentItemName READ getCurrentItemName NOTIFY currentIndexChanged)
    Q_PROPERTY(ServerListModel *serverList READ getServerList CONSTANT)
    Q_PROPERTY(SubtitleListModel *subtitleList READ getSubtitleList CONSTANT)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)

private:
    bool m_isLoading = false;
    void setIsLoading(bool value);
    std::atomic<bool> m_isCancelled = false;
    Client m_client = Client(&m_isCancelled);

    PlaylistItem *m_currentLoadingEpisode = nullptr;
    QFileSystemWatcher m_folderWatcher;
    QFutureWatcher<PlayInfo> m_watcher;
    ServerListModel m_serverListModel;
    SubtitleListModel m_subtitleListModel;


    PlaylistItem *m_root = new PlaylistItem("root", nullptr, "/");
    QSet<QString> playlistSet; // Prevents the playlist with the same being added

    ServerListModel *getServerList() { return &m_serverListModel; }
    SubtitleListModel *getSubtitleList() { return &m_subtitleListModel; }

    QString getCurrentItemName() const;
    QModelIndex getCurrentIndex() const;

    bool registerPlaylist(PlaylistItem *playlist);
    void unregisterPlaylist(PlaylistItem *playlist);

    PlayInfo play(int playlistIndex, int itemIndex);
    Q_SLOT void onLocalDirectoryChanged(const QString &path);
    QStringList m_subtitleExtensions = { "srt", "sub", "ssa", "ass", "idx", "vtt" };
    void setSubtitle(const QUrl &url);
public:
    explicit PlaylistManager(QObject *parent = nullptr);
    ~PlaylistManager() { delete m_root; }

    void appendPlaylist(PlaylistItem *playlist);
    Q_INVOKABLE void appendFolderPlaylist(const QUrl &url) {
        if (m_subtitleExtensions.contains(QFileInfo(url.path()).suffix())) {
            setSubtitle(url);
        } else {
            PlaylistItem *playlist = PlaylistItem::fromLocalUrl(url);
            appendPlaylist(playlist);
        }
    }



    void replaceMainPlaylist(PlaylistItem *playlist);
    void replacePlaylistAt(int index, PlaylistItem *newPlaylist);
    Q_INVOKABLE void removePlaylistAt(int index) {
        if (index == m_root->currentIndex) {
            return;
        }
        auto playlistToRemove = m_root->at(index);
        if (!playlistToRemove) return;
        auto currentPlaylist = m_root->getCurrentItem();

        unregisterPlaylist(playlistToRemove);
        beginRemoveRows(QModelIndex(), index, index);
        m_root->removeOne(playlistToRemove);
        m_root->currentIndex = m_root->indexOf(currentPlaylist);
        endRemoveRows();

    }
    Q_INVOKABLE void clear() {
        beginRemoveRows(QModelIndex(), 0, m_root->size() - 1);
        auto currentPlaylist = m_root->getCurrentItem();
        //qDebug() << "current" << currentPlaylist->name;
        for (const auto &playlist : *m_root->children()) {
            if (playlist == currentPlaylist) {
                //qDebug() << playlist->name;
                continue;
            }
            unregisterPlaylist(playlist);
            m_root->removeOne(playlist);
        }
        endRemoveRows();
        beginInsertRows(QModelIndex(), 0, 0);
        endInsertRows();
        m_root->currentIndex = m_root->isEmpty() ? -1 : 0;

        //qDebug() << "new current" << m_root->getCurrentItem()->name;
        emit currentIndexChanged();

    }


    PlaylistItem *findPlaylist(const QString &link) {
        if (!playlistSet.contains (link)) return nullptr;
        return m_root->at (m_root->indexOf (link));
    }

    PlaylistItem *getCurrentPlaylist() const { return m_root->getCurrentItem(); }

    Q_INVOKABLE void openUrl(const QUrl &url, bool playUrl);
    Q_INVOKABLE void loadIndex(QModelIndex index);
    Q_INVOKABLE void pasteOpen(QString path);
    Q_INVOKABLE void loadServer(int index);
    Q_INVOKABLE void reload();

    QModelIndex getCurrentListIndex() {
        return createIndex (m_root->currentIndex, 0, m_root->getCurrentItem());
    }

    bool isLoading() { return m_isLoading; }

    //  Traversing the playlist
    Q_INVOKABLE bool tryPlay(int playlistIndex = -1, int itemIndex = -1);
    Q_INVOKABLE void loadOffset(int offset);
    Q_INVOKABLE inline void playNextItem() { loadOffset(1); }
    Q_INVOKABLE inline void playPrecedingItem() { loadOffset(-1); }
    Q_INVOKABLE void cancel() {
        if (m_watcher.isRunning()) {
            m_isCancelled = true;
        }
    }

    Q_SIGNAL void isLoadingChanged(void);
    Q_SIGNAL void currentIndexChanged(void);
    Q_SIGNAL void aboutToPlay(void);


private:
    enum
    {
        TitleRole = Qt::UserRole,
        IndexRole,
        NumberRole,
        NumberTitleRole,
        IsCurrentIndexRole,
        IndexInParentRole
    };
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &childIndex) const override;
    int columnCount(const QModelIndex &parent) const override { return 1; }
};

