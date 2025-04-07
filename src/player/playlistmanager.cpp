#include "playlistmanager.h"
#include "utils/logger.h"
#include "utils/myexception.h"
#include "player/mpvObject.h"
#include "utils/errorhandler.h"
#include "providers/showprovider.h"

PlaylistManager::PlaylistManager(QObject *parent) : QAbstractItemModel(parent)
{
    // Opens the file to play immediately when application launches

    connect (&m_folderWatcher, &QFileSystemWatcher::directoryChanged, this, &PlaylistManager::onLocalDirectoryChanged);
    connect (&m_watcher, &QFutureWatcher<PlayItem>::finished, this, &PlaylistManager::onLoadFinished);
    // about to start
    connect (&m_watcher, &QFutureWatcher<PlayItem>::started, this, [this](){
        m_isCancelled = false;
        setIsLoading(true);
    });
}

void PlaylistManager::onLoadFinished() {
    if (!m_isCancelled.load()) {
        try {
            setCurrentPlayItem(m_watcher.result());
            MpvObject::instance()->setHeaders(m_currentPlayItem.headers);

            auto audioUrl = m_currentPlayItem.audios.isEmpty() ? QUrl() : m_currentPlayItem.audios.first().url;

            MpvObject::instance()->open(m_currentPlayItem.videos.first().url,
                                        audioUrl,
                                        m_currentPlayItem.timeStamp);

            emit aboutToPlay();
        } catch (MyException& ex) {
            ex.show();
        } catch(const std::runtime_error& ex) {
            ErrorHandler::instance().show (ex.what(), "Playlist Error");
        } catch (...) {
            ErrorHandler::instance().show ("Something went wrong", "Playlist Error");
        }
    }
    setIsLoading(false);
    m_isCancelled = false;

    // TODO connect to see if opened successfully as it might lose timestamp from switching servers

}

bool PlaylistManager::tryPlay(int playlistIndex, int itemIndex) {
    if (m_watcher.isRunning()) {
        m_isCancelled = true;
        return false;
    }

    playlistIndex = playlistIndex == -1 ? (m_root->currentIndex == -1 ? 0 : m_root->currentIndex) : playlistIndex;
    auto newPlaylist = m_root->at(playlistIndex);
    if (!newPlaylist) return false;

    // Set to current playlist item index if -1
    itemIndex = itemIndex == -1 ? (newPlaylist->currentIndex == -1 ? 0 : newPlaylist->currentIndex) : itemIndex;
    if (!newPlaylist->isValidIndex(itemIndex) || newPlaylist->at (itemIndex)->type == PlaylistItem::LIST) {
        oLog() << "Playlist" << "Invalid index or attempting to play a list";
        return false;
    }

    m_isCancelled = false;
    setIsLoading(true);
    if (m_root->currentIndex != playlistIndex)
        emit currentIndexAboutToChange();

    m_watcher.setFuture(QtConcurrent::run(&PlaylistManager::play, this, playlistIndex, itemIndex));
    return true;

}

PlayItem PlaylistManager::play(int playlistIndex, int itemIndex) {

    auto playlist = m_root->at(playlistIndex);
    auto episode = playlist->at(itemIndex);

    PlayItem playItem;

    cLog() << "Playlist" << "Timestamp:" << playlist->at(itemIndex)->timeStamp;

    if (episode->type == PlaylistItem::PASTED) {
        playItem.videos.emplaceBack(episode->link);
    }
    else if (episode->type == PlaylistItem::LOCAL) {
        if (!QDir(playlist->link).exists()) {
            beginResetModel();
            m_root->children()->removeOne(playlist);
            unregisterPlaylist(playlist);
            m_root->currentIndex = m_root->isEmpty() ? -1 : 0;
            endResetModel();
            return playItem;
        }

        if(playlist->currentIndex != itemIndex){
            playlist->currentIndex = itemIndex;
            playlist->updateHistoryFile(0);
        }
        playItem.videos.emplaceBack(episode->link);
    }
    else {
        auto provider = playlist->getProvider();
        if (!provider) throw MyException("Cannot get provider from playlist!", "Provider");

        auto episodeName = episode->getFullName().trimmed().replace('\n', " ");

        // Load server list
        auto servers = provider->loadServers(&m_client, episode);
        if (servers.isEmpty()) throw MyException("No servers found for " + episodeName, "Server");

        // Sort servers by name
        std::sort(servers.begin(), servers.end(), [](const VideoServer &a, const VideoServer &b) {
            return a.name < b.name;
        });

        // Find a working server
        auto result = ServerListModel::findWorkingServer(&m_client, provider, servers);
        if (result.first == -1) throw MyException("No working server found for " + episodeName, "Server");

        if (m_isCancelled.load()) return {};
        // Set the servers and the index of the working server
        m_serverListModel.setServers(servers, provider);
        m_serverListModel.setCurrentIndex(result.first);
        m_serverListModel.setPreferredServer(result.first);
        playItem = result.second;
    }
    if (m_isCancelled.load()) return {};

    playItem.timeStamp = episode->timeStamp;

    m_root->currentIndex = playlistIndex;
    playlist->currentIndex = itemIndex;
    emit currentIndexChanged();
    return playItem;
}

void PlaylistManager::loadServer(int index) {
    if (m_watcher.isRunning()) {
        m_isCancelled = true;
        return;
    }
    if (!m_serverListModel.isValidIndex(index)) return;

    m_watcher.setFuture(QtConcurrent::run([&, index](){
        auto client = Client(&m_isCancelled);
        auto serverName = m_serverListModel.getServerAt(index).name;
        PlayItem playItem = m_serverListModel.loadServer(&client, index);
        if (playItem.videos.isEmpty()) {
            throw MyException(QString("Failed to load server %1").arg(serverName), "Server");
        }
        playItem.timeStamp = MpvObject::instance()->time();
        m_serverListModel.setCurrentIndex(index);
        m_serverListModel.setPreferredServer(index);

        return playItem;
    }));
}

void PlaylistManager::loadIndex(QModelIndex index) {
    auto childItem = static_cast<PlaylistItem *>(index.internalPointer());
    auto parentItem = childItem->parent();
    if (parentItem == m_root) return;
    int itemIndex = childItem->row();
    int playlistIndex = m_root->indexOf(parentItem);
    tryPlay(playlistIndex, itemIndex);
}

void PlaylistManager::loadOffset(int offset) {
    auto currentPlaylist = m_root->getCurrentItem();
    if (!currentPlaylist) return;
    int newIndex = currentPlaylist->currentIndex + offset;

    if (!currentPlaylist->isValidIndex(newIndex)) return;

    auto time = MpvObject::instance()->time();

    if (currentPlaylist->currentIndex != currentPlaylist->size() - 1 && time > 0.96 * MpvObject::instance()->duration()){
        currentPlaylist->getCurrentItem()->timeStamp = 0;
    } else {
        currentPlaylist->getCurrentItem()->timeStamp = time;
    }

    tryPlay(m_root->currentIndex, newIndex);
}

void PlaylistManager::onLocalDirectoryChanged(const QString &path) {
    int index = m_root->indexOf(path);
    if (index == -1)  return;
    beginResetModel();
    QString prevlink;
    if (m_root->currentIndex == index && m_root->getCurrentItem()->currentIndex != -1) {
        prevlink = m_root->getCurrentItem()->getCurrentItem()->link;
    }
    // doesnt work
    if (!m_root->at(index)->reloadFromFolder()) {
        // Folder is empty, deleted, can't open history file etc.
        m_root->removeAt(index);
        m_root->currentIndex = m_root->isEmpty() ? -1 : 0;
        cLog() << "Playlist" << "Failed to reload folder" << m_root->at(index)->link;
    }
    endResetModel();
    emit currentIndexChanged();

    if (m_root->currentIndex == index && m_root->getCurrentItem()->currentIndex != -1) {
        QString newLink = m_root->getCurrentItem()->getCurrentItem()->link;
        if (prevlink != newLink) {
            tryPlay();
        }
    }
}



void PlaylistManager::setSubtitle(const QUrl &url) {
    MpvObject::instance()->showText (QString("Setting subtitle: %1").arg(url.toEncoded()));
    MpvObject::instance()->addSubtitle(url);
    MpvObject::instance()->setSubVisible(true);
}

bool PlaylistManager::registerPlaylist(PlaylistItem *playlist) {
    if (!playlist || playlistSet.contains(playlist->link)) return false;
    playlist->use();
    playlistSet.insert(playlist->link);

    // Watch playlist path if local folder
    if (playlist->isLoadedFromFolder()) {
        m_folderWatcher.addPath(playlist->link);
    }
    return true;
}

void PlaylistManager::unregisterPlaylist(PlaylistItem *playlist) {
    if (!playlist || !playlistSet.contains(playlist->link)) return;
    playlistSet.remove(playlist->link);
    // Unwatch playlist path if local folder
    if (playlist->isLoadedFromFolder()) {
        m_folderWatcher.removePath(playlist->link);
    }
    playlist->disuse();
}

int PlaylistManager::append(PlaylistItem *playlist) {
    if (!registerPlaylist(playlist)) {
        return m_root->indexOf(playlist->link);
    }
    auto row = m_root->size();
    beginInsertRows(QModelIndex(), row, row);
    m_root->append(playlist);
    endInsertRows();
    return row;
}

int PlaylistManager::insert(int index, PlaylistItem *playlist) {
    if (index < 0 || index >= m_root->size()) {
        return append(playlist);
    }
    if (!registerPlaylist(playlist)) {
        return m_root->indexOf(playlist->link);
    }
    beginInsertRows(QModelIndex(), index, index);
    m_root->insert(index, playlist);
    endInsertRows();
    return index;
}



int PlaylistManager::replace(int index, PlaylistItem *newPlaylist) {
    if (m_root->isEmpty() || index < 0 || index >= m_root->size()) {
        return append(newPlaylist);
    }
    if (!registerPlaylist(newPlaylist)) {
        return m_root->indexOf(newPlaylist->link);
    }
    auto playlistToReplace = m_root->at(index);
    if (!newPlaylist || !playlistToReplace) return -1;

    beginRemoveRows(QModelIndex(), index, index);
    unregisterPlaylist(playlistToReplace);
    endRemoveRows();
    beginInsertRows(QModelIndex(), index, index);
    m_root->replace(index, newPlaylist);
    endInsertRows();

    return index;
}

void PlaylistManager::removeAt(int index) {
    // Validate index and ensure we're not removing the currently playing playlist
    if (index < 0 || index >= m_root->size() || index == m_root->currentIndex) {
        return;
    }

    PlaylistItem *playlistToRemove = m_root->at(index);
    if (!playlistToRemove) return;

    // Storing currentPlaylist before removal
    PlaylistItem *currentPlaylist = m_root->getCurrentItem();

    // Begin removal operation
    beginRemoveRows(QModelIndex(), index, index);
    unregisterPlaylist(playlistToRemove);
    m_root->removeAt(index);
    endRemoveRows();

    // currentPlaylist is still be valid, but its index might have changed.
    if (currentPlaylist) {
        int newCurrentIndex = m_root->indexOf(currentPlaylist);
        m_root->currentIndex = newCurrentIndex;
        emit currentIndexChanged();
    }
}


void PlaylistManager::clear() {
    beginRemoveRows(QModelIndex(), 0, m_root->size() - 1);
    auto currentPlaylist = m_root->getCurrentItem();
    for (int i = 0; i < m_root->size(); i++) {
        const auto &playlist = m_root->at(i);
        if (playlist == currentPlaylist) continue;
        unregisterPlaylist(playlist);
        m_root->children()->removeOne(playlist);
    }
    endRemoveRows();
    beginInsertRows(QModelIndex(), 0, 0);
    endInsertRows();
    m_root->currentIndex = m_root->isEmpty() ? -1 : 0;
    emit currentIndexChanged();

}

void PlaylistManager::openUrl(QUrl url, bool play) {
    QString urlString = url.toString();
    if (url.isEmpty()) {
        urlString = QGuiApplication::clipboard()->text().trimmed();
        if ((urlString.startsWith('\'') && urlString.endsWith('\'')) ||
            (urlString.startsWith('"') && urlString.endsWith('"'))
            ) {
            urlString.removeAt(0);
            urlString.removeLast();
        }
        urlString.replace("\\/", "/");
        url = QUrl::fromUserInput(urlString);
    }

    if (!url.isValid()) return;

    static QStringList m_subtitleExtensions = { "srt", "sub", "ssa", "ass", "idx", "vtt" };
    if (m_subtitleExtensions.contains(QFileInfo(url.path()).suffix()) || url.path().toLower().contains("subtitle") ) {
        setSubtitle(url);
        return;
    }

    // static QRegularExpression urlPattern(R"(https?:\/\/(?:www\.|(?!www))[a-zA-Z0-9][a-zA-Z0-9-]+[a-zA-Z0-9]\.[^\s]{2,}|www\.[a-zA-Z0-9][a-zA-Z0-9-]+[a-zA-Z0-9]\.[^\s]{2,}|https?:\/\/(?:www\.|(?!www))[a-zA-Z0-9]+\.[^\s]{2,}|www\.[a-zA-Z0-9]+\.[^\s]{2,})");
    int playlistIndex = -1;
    if (url.isLocalFile()) {
        cLog() << "Playlist" << "Opening local file" << url;
        playlistIndex = append(PlaylistItem::fromLocalUrl(url));
    } else { // Online video

        // TODO connect to MPV and check if error emitted,


        // if (!m_client.isOk(urlString)) {
        //     MpvObject::instance()->showText(QString("Invalid url: %1").arg(urlString.toUtf8()));
        //     oLog() << "Playlist" << "Invalid url:" << url;
        //     return;
        // }

        cLog() << "Playlist" << "Opening online video" << urlString;

        playlistIndex = m_root->indexOf("videos");

        if (playlistIndex == -1) {
            // create a playlist for pasted videos
            playlistIndex = append(new PlaylistItem("Videos", nullptr, "videos"));
        }

        auto pastePlaylist = m_root->at(playlistIndex);
        auto itemIndex = pastePlaylist->indexOf(urlString);
        if (itemIndex == -1) {
            auto parent = createIndex(playlistIndex, 0, pastePlaylist);
            beginInsertRows(parent, pastePlaylist->size(), pastePlaylist->size());
            pastePlaylist->emplaceBack (0, pastePlaylist->size() + 1, urlString, urlString, true);
            endInsertRows();
            pastePlaylist->last()->type = PlaylistItem::PASTED;
            m_root->at(playlistIndex)->currentIndex = pastePlaylist->size() - 1;
        } else {
            m_root->at(playlistIndex)->currentIndex = itemIndex;
        }

    }

    if (play && MpvObject::instance()->getCurrentVideoUrl() != url && playlistIndex != -1) {
        MpvObject::instance()->showText(QString("Playing: %1").arg(urlString.toUtf8()));
        tryPlay(playlistIndex);
    }

}

void PlaylistManager::reload() {
    auto currentPlaylist = m_root->getCurrentItem();
    if (!currentPlaylist) return;
    auto time = MpvObject::instance()->time();
    currentPlaylist->setLastPlayAt(currentPlaylist->currentIndex, time);
    tryPlay();
}

void PlaylistManager::setIsLoading(bool value) {
    m_isLoading = value;
    emit isLoadingChanged();
}



void PlaylistManager::loadVideo(int index) {
    if (index < 0 && index >= m_currentPlayItem.videos.size()) return;
    auto currentAudio = m_currentPlayItem.audios.isEmpty() ? QUrl() : m_currentPlayItem.audios[m_audioListModel.getCurrentIndex()].url;
    MpvObject::instance()->open(m_currentPlayItem.videos[index].url,
                                currentAudio,
                                MpvObject::instance()->time());
    m_videoListModel.setCurrentIndex(index);
}

void PlaylistManager::loadAudio(int index) {
    if (index < 0 && index >= m_currentPlayItem.audios.size()) return;
    MpvObject::instance()->addAudioTrack(m_currentPlayItem.audios[index].url);
    m_audioListModel.setCurrentIndex(index);
}

void PlaylistManager::showCurrentItemName() const {
    auto currentPlaylist = m_root->getCurrentItem();
    if (!currentPlaylist) return;
    auto itemName = currentPlaylist->getDisplayNameAt(currentPlaylist->currentIndex);
    MpvObject::instance()->showText(itemName);
}

QModelIndex PlaylistManager::getCurrentModelIndex() const {
    PlaylistItem *currentPlaylist = m_root->getCurrentItem();
    if (!currentPlaylist ||
        !currentPlaylist->isValidIndex(currentPlaylist->currentIndex))
        return QModelIndex();

    return index(currentPlaylist->currentIndex, 0, index(m_root->currentIndex, 0, QModelIndex()));
}

int PlaylistManager::rowCount(const QModelIndex &parent) const {
    if (parent.column() > 0) return 0;

    const PlaylistItem *parentItem = parent.isValid() ? static_cast<PlaylistItem*>(parent.internalPointer()) : m_root;
    return parentItem->size();
}

QModelIndex PlaylistManager::index(int row, int column, const QModelIndex &parent) const {
    if (!hasIndex(row, column, parent)) return QModelIndex();
    PlaylistItem *parentItem = parent.isValid() ? static_cast<PlaylistItem *>(parent.internalPointer()) : m_root;
    PlaylistItem *childItem = parentItem->at(row);
    return childItem ? createIndex(row, column, childItem) : QModelIndex();
}

QModelIndex PlaylistManager::parent(const QModelIndex &childIndex) const {
    if (!childIndex.isValid()) return QModelIndex();

    PlaylistItem *childItem = static_cast<PlaylistItem *>(childIndex.internalPointer());
    PlaylistItem *parentItem = childItem->parent();

    if (parentItem == m_root || parentItem == nullptr) return QModelIndex();
    // Q_ASSERT(parentItem);
    return createIndex(parentItem->row(), 0, parentItem);
}

QVariant PlaylistManager::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || m_root->isEmpty())
        return QVariant();
    auto item = static_cast<PlaylistItem*>(index.internalPointer());

    switch (role) {
    case TitleRole:
        return item->name;
        break;
    case IndexRole:
        return index;
        break;
    case NumberRole:
        return item->number;
        break;
    case NumberTitleRole: {
        if (item->type == 0) {
            return item->name;
        }
        return item->getFullName();
        break;
    case IsCurrentIndexRole:
        if (!item->parent() || item->parent()->currentIndex == -1) return false;
        return item->parent()->getCurrentItem() == item;
        break;
    }
    default:
        break;
    }
    return QVariant();
}

QHash<int, QByteArray> PlaylistManager::roleNames() const {
    QHash<int, QByteArray> names {
        {TitleRole, "title"},
        {NumberRole, "number"},
        {IndexRole, "index"},
        {NumberTitleRole, "numberTitle"},
        {IsCurrentIndexRole, "isCurrentIndex"},
    };
    return names;
}
