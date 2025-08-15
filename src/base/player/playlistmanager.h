#pragma once
#include <QDir>
#include <QFileSystemWatcher>
#include <QStandardItemModel>
#include "base/showdata.h"
#include "gui/serverlistmodel.h"
#include "playlistitem.h"
#include "base/servicemanager.h"


#include <QtConcurrent> //to remove

class PlaylistManager : public ServiceManager {
    Q_OBJECT
    Q_PROPERTY(ServerListModel *serverList READ getServerList CONSTANT)
public:
    explicit PlaylistManager(QObject *parent = nullptr);
    ~PlaylistManager() { delete m_root; }

    PlaylistItem *root() const { return m_root; }
    PlaylistItem *find(const QString &link);
    int count() const { return m_root->count(); }

    int append(PlaylistItem *playlist, PlaylistItem *parent = nullptr) { return insert(INT_MAX, playlist, parent); }
    int insert(int index, PlaylistItem *playlist, PlaylistItem *parent = nullptr);
    int replace(int index, PlaylistItem *playlist, PlaylistItem *parent = nullptr);
    Q_INVOKABLE void remove(QModelIndex index);
    Q_INVOKABLE void clear();

    //  Traversing the playlist
    Q_INVOKABLE bool tryPlay(int playlistIndex = -1, int itemIndex = -1);

    Q_INVOKABLE bool tryPlay(PlaylistItem *item);

    Q_INVOKABLE void loadNextItem(int offset = 1);
    Q_INVOKABLE void loadNextPlaylist(int offset = 1);
    Q_INVOKABLE void loadIndex(const QModelIndex &index);
    Q_INVOKABLE void reload();
    Q_INVOKABLE void openUrl(QUrl url, bool play);
    Q_INVOKABLE void loadServer(int index);

    Q_INVOKABLE void showCurrentItemName() const;
    Q_INVOKABLE void saveProgress() const;

    // void updateSelection(bool scrollToIndex = false);
    Q_INVOKABLE void cancel();

    // Signals
    Q_SIGNAL void aboutToInsert(PlaylistItem *parent, int index);
    Q_SIGNAL void inserted();
    Q_SIGNAL void aboutToRemove(PlaylistItem *item);
    Q_SIGNAL void removed();
    Q_SIGNAL void changed(int playlistIndex);
    Q_SIGNAL void updateSelections(PlaylistItem *currentItem, bool scrollToIndex = false); //int playlistIndex, int itemIndex,
    Q_SIGNAL void modelReset();

    Q_SIGNAL void progressUpdated(QString link, int progressIndex, int timestamp) const;
    Q_SIGNAL void aboutToPlay(void) const;
private:
    PlaylistItem *m_root = new PlaylistItem("root", nullptr, "/");
    Client m_client = Client(&m_isCancelled);

    PlayInfo play(PlaylistItem *item) {
        if (!item) return {};
        PlaylistItem *playlist;

        if (item->type == PlaylistItem::LIST) {
            // Attempting to play a list
            if (item->isEmpty()) return {};
            auto currentItem = item->getCurrentItem();
            playlist = item;
            item = currentItem ? currentItem : item->at(0);
        } else {
            playlist = item->parent();
        }

        if (!playlist || playlist->type != PlaylistItem::LIST) {
            rLog() << "Playlist" << item->name << "does not belong to any playlist!";
            return {};
        }
        PlayInfo playInfo;
        m_serverListModel.clear();
        auto itemRow = item->row();

        switch(item->type) {
        case PlaylistItem::PASTED: {
            if (item->link.contains('|')) {
                // curl command
                QStringList parts = item->link.split('|');
                playInfo.videos.emplaceBack(parts.takeFirst());
                for (const QString &headerLine : std::as_const(parts)) {
                    QStringList keyValue = headerLine.split(": ", Qt::KeepEmptyParts);
                    if (keyValue.size() == 2) {
                        playInfo.headers.insert(keyValue[0].trimmed(), keyValue[1].trimmed());
                    }
                }
            } else {
                playInfo.videos.emplaceBack(item->link);
            }
            break;
        }
        case PlaylistItem::ONLINE: {
            auto provider = playlist->getProvider();
            if (!provider)
                throw MyException("Cannot get provider from playlist!", "Provider");

            auto episodeName = item->displayName.trimmed().replace('\n', " ");

            // Load server list
            auto servers = provider->loadServers(&m_client, item);
            if (servers.isEmpty())
                throw MyException("No servers found for " + episodeName, "Server");

            // Sort servers by name
            std::sort(servers.begin(), servers.end(),
                      [](const VideoServer &a, const VideoServer &b) {
                          return a.name < b.name;
                      });

            // Find a working server
            auto result = ServerListModel::findWorkingServer(&m_client, provider, servers);
            if (result.first == -1)
                throw MyException("No working server found for " + episodeName, "Server");

            if (m_isCancelled) return {};
            // Set the servers and the index of the working server
            m_serverListModel.setServers(servers, provider);
            m_serverListModel.setCurrentIndex(result.first);
            m_serverListModel.setPreferredServer(result.first);
            playInfo = result.second;
            break;
        }
        case PlaylistItem::LOCAL: {
            if (!QDir(item->link).exists()) {
                // In case localdirectory change doesn't catch this
                auto parent = playlist->parent();
                Q_ASSERT(parent);
                aboutToRemove(item);
                parent->removeOne(playlist);
                parent->setCurrentIndex(parent->isEmpty() ? -1 : 0);
                emit modelReset();
                return {};
            }
            playInfo.videos.emplaceBack(item->link);
            break;
        }
        case PlaylistItem::LIST:
            break;
        }

        if (m_isCancelled) return {};
        if (playlist->getCurrentIndex() != itemRow) {
            playlist->setCurrentIndex(itemRow);
            playlist->updateHistoryFile();
        }
        auto parent = playlist;
        auto row = itemRow;
        while (parent) {
            parent->setCurrentIndex(row);
            row = parent->row();
            parent = parent->parent();
        }
        playInfo.timeStamp = item->getTimestamp();

        emit progressUpdated(playlist->link, itemRow, item->getTimestamp());
        // updateSelection(true);
        m_currentItem = item;
        emit updateSelections(m_currentItem, true);
        return playInfo;
    }

    PlaylistItem *m_currentItem = nullptr;
    void setCurrentItem(PlaylistItem *currentItem) {
        if (!currentItem) {
            m_currentItem = nullptr;
            return;
        }
        if (currentItem->isList()) return;
        m_currentItem = currentItem;
        int row = currentItem->row();
        auto parent = currentItem->parent();
        while (parent) {
            parent->setCurrentIndex(row);
            row = parent->row();
            parent = parent->parent();
        }
    }

    // Watchers
    QFileSystemWatcher m_folderWatcher;
    QFutureWatcher<PlayInfo> m_watcher;

    // models
    ServerListModel m_serverListModel;
    ServerListModel *getServerList() { return &m_serverListModel; }

    // Prevents the playlist with the same link being added
    QMap<QString, PlaylistItem*> m_playlistMap;
    void registerPlaylist(PlaylistItem *playlist);
    void deregisterPlaylist(PlaylistItem *playlist);

    PlaylistItem *loadFromFolder(const QUrl &pathUrl, PlaylistItem *playlist = nullptr);

    Q_SLOT void onLocalDirectoryChanged(const QString &path);
    Q_SLOT void onLoadFinished();
private:
    PlayInfo m_currentPlayItem;
};

