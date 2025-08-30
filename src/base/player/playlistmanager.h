#pragma once
#include <QDir>
#include <QFileSystemWatcher>
#include <QStandardItemModel>
#include <QSharedPointer>
#include <QWeakPointer>
#include "base/showdata.h"
#include "ui/models/serverlistmodel.h"
#include "playlistitem.h"
#include "base/servicemanager.h"

class PlaylistManager : public ServiceManager {
    Q_OBJECT
    Q_PROPERTY(ServerListModel *serverList READ getServerList CONSTANT)
public:
    explicit PlaylistManager(QObject *parent = nullptr);
    ~PlaylistManager();;

    QSharedPointer<PlaylistItem> root() const { return m_root; }
    QSharedPointer<PlaylistItem> find(const QString &link);
    int count() const { return m_root->count(); }
    int append(QSharedPointer<PlaylistItem> playlist, QSharedPointer<PlaylistItem> parent = nullptr);
    int insert(int index, QSharedPointer<PlaylistItem> playlist, QSharedPointer<PlaylistItem> parent = nullptr);
    int replace(int index, QSharedPointer<PlaylistItem> playlist, QSharedPointer<PlaylistItem> parent = nullptr);
    Q_INVOKABLE void remove(const QModelIndex &index);
    Q_INVOKABLE void clear();

    Q_INVOKABLE bool playPlaylist(int index);
    Q_INVOKABLE void loadNextItem(int offset = 1);
    Q_INVOKABLE void loadNextPlaylist(int offset = 1);
    Q_INVOKABLE void loadIndex(const QModelIndex &index);
    Q_INVOKABLE void reload();
    Q_INVOKABLE void openUrl(QUrl url, bool play);
    Q_INVOKABLE void loadServer(int index);

    Q_INVOKABLE void showCurrentItemName() const;
    Q_INVOKABLE void saveProgress() const;
    Q_INVOKABLE void cancel();

    Q_SIGNAL void aboutToInsert(PlaylistItem* parent, int index);
    Q_SIGNAL void inserted();
    Q_SIGNAL void aboutToRemove(PlaylistItem* item);
    Q_SIGNAL void removed();
    Q_SIGNAL void dataChanged(PlaylistItem* item);
    Q_SIGNAL void updateSelections(PlaylistItem* currentItem); 
    Q_SIGNAL void scrollToCurrentIndex();
    Q_SIGNAL void modelReset();
    Q_SIGNAL void progressUpdated(QString link, int progressIndex, int timestamp) const;

private:
    QSharedPointer<PlaylistItem> m_root = QSharedPointer<PlaylistItem>::create("root", nullptr, "/");
    Client m_client = Client { &m_cancelled };
    QFileSystemWatcher m_folderWatcher;
    QFutureWatcher<PlayInfo> m_watcher;
    ServerListModel m_serverListModel;
    QWeakPointer<PlaylistItem> m_currentItem;
    QMap<QString, QWeakPointer<PlaylistItem>> m_playlistMap;
    QStringList m_playableExtensions {"*.mp4", "*.mkv", "*.avi", "*.mp3", "*.flac", "*.wav", "*.ogg", "*.webm", "*.m3u8", "*.mov"};

    bool tryPlay(QSharedPointer<PlaylistItem> item);
    PlayInfo play(QSharedPointer<PlaylistItem> item);
    void setCurrentItem(QSharedPointer<PlaylistItem> currentItem);
    ServerListModel *getServerList() { return &m_serverListModel; }
    bool loadFromFolder(const QUrl &pathUrl, QSharedPointer<PlaylistItem> playlist, bool recursive = true);
    Q_SLOT void onLocalDirectoryChanged(const QString &path);

    // Prevents the playlist with the same link being added
    void registerPlaylist(QSharedPointer<PlaylistItem> playlist);
    void deregisterPlaylist(QSharedPointer<PlaylistItem> playlist);

    
};

