#include "playlistmanager.h"
#include "network/myexception.h"
#include "player/mpvObject.h"
#include "utils/errorhandler.h"
#include "providers/showprovider.h"

PlaylistManager::PlaylistManager(QObject *parent){
    // Opens the file to play immediately when application launches

    connect (&m_folderWatcher, &QFileSystemWatcher::directoryChanged, this, &PlaylistManager::onLocalDirectoryChanged);

    connect (&m_watcher, &QFutureWatcher<PlayInfo>::finished, this, [this](){
        if (!m_isCancelled) {
            try {
                auto playInfo = m_watcher.result();
                if (playInfo.sources.isEmpty()) return;
                MpvObject::instance()->open(playInfo.sources.first(), m_currentLoadingEpisode->timeStamp);
                emit aboutToPlay();
            } catch (MyException& ex) {
                ErrorHandler::instance().show (ex.what(), "Playlist MyError");
            } catch(const std::runtime_error& ex) {
                ErrorHandler::instance().show (ex.what(), "Playlist Error");
            } catch (...) {
                ErrorHandler::instance().show ("Something went wrong", "Playlist Error");
            }
        }
        setIsLoading(false);
        m_currentLoadingEpisode = nullptr;
        m_isCancelled = false;
    });


}

bool PlaylistManager::tryPlay(int playlistIndex, int itemIndex) {
    if (m_watcher.isRunning()) {
        m_isCancelled = true;
        return false;
    }

    playlistIndex = playlistIndex == -1 ? (m_root->currentIndex == -1 ? 0 : m_root->currentIndex) : playlistIndex;
    auto newPlaylist = m_root->at (playlistIndex);
    if (!newPlaylist) return false;

    // Set to current playlist item index if -1
    itemIndex = itemIndex == -1 ? (newPlaylist->currentIndex == -1 ? 0 : newPlaylist->currentIndex) : itemIndex;
    if (!newPlaylist->isValidIndex (itemIndex) || newPlaylist->at (itemIndex)->type == PlaylistItem::LIST) {
        qWarning() << "Invalid index or attempting to play a list";
        return false;
    }
    auto episodeToLoad = m_root->at(playlistIndex)->at(itemIndex);
    if (episodeToLoad == m_currentLoadingEpisode) {
        qDebug() << "same episode!";
    }

    m_isCancelled = false;
    setIsLoading(true);
    m_currentLoadingEpisode = episodeToLoad;
    m_watcher.setFuture(QtConcurrent::run(&PlaylistManager::play, this, playlistIndex, itemIndex));
    return true;

}

PlayInfo PlaylistManager::play(int playlistIndex, int itemIndex) {
    auto currentPlaylist = m_root->getCurrentItem();
    auto playlist = m_root->at(playlistIndex);
    auto episode = playlist->at(itemIndex);
    PlayInfo playInfo;

    if (currentPlaylist == playlist && currentPlaylist->currentIndex != -1 && currentPlaylist->currentIndex != itemIndex) {
        auto time = MpvObject::instance()->time();
        if (time > 0.85 * MpvObject::instance()->duration())
            time = 0;
        qInfo() << "Log (Playlist)   : Saving timestamp" << time << "for" << currentPlaylist->link;
        currentPlaylist->setLastPlayAt(currentPlaylist->currentIndex, time);
    }

    qDebug() << "Log (Playlist)   : Timestamp:" << playlist->at(itemIndex)->timeStamp;


    if (episode->type == PlaylistItem::LOCAL) {
        if (!QDir(playlist->link).exists()) {
            beginResetModel();
            m_root->removeOne(playlist);
            unregisterPlaylist(playlist);
            m_root->currentIndex = m_root->isEmpty() ? -1 : 0;
            endResetModel();
            return playInfo;
        }
        if (playlist->currentIndex != itemIndex){
            playlist->currentIndex = itemIndex;
            playlist->updateHistoryFile(0);
        }
        m_subtitleListModel.clear();
        playInfo.sources.emplaceBack (episode->link);

    } else {
        ShowProvider *provider = playlist->getProvider();
        if (!provider){
            throw MyException("Cannot get provider from playlist!");
        }


        qInfo().noquote() << QString("Log (Playlist)   : Fetching servers for %1 [%2/%3]")
                                 .arg (episode->getFullName().trimmed()).arg (itemIndex + 1).arg (playlist->size());

        QList<VideoServer> servers = provider->loadServers(&m_client, episode);
        if (m_isCancelled) return {};
        if (servers.isEmpty()) {
            throw MyException("No servers found for " + episode->getFullName().trimmed());
        }

        qInfo().noquote() << "Log (Playlist)   : Successfully fetched servers for " << episode->getFullName().trimmed();

        if (m_isCancelled) return {};
        playInfo = ServerListModel::autoSelectServer(&m_client, servers, provider);
        if (m_isCancelled) return {};
        if (!playInfo.sources.isEmpty()) {
            m_serverListModel.setServers(servers, provider);
            m_serverListModel.setCurrentIndex(playInfo.serverIndex);
            m_subtitleListModel.setList(playInfo.subtitles);
        } else {
            throw MyException("No sources extracted from " + episode->getFullName().trimmed());
        }
    }
    if (m_isCancelled) return {};

    // If same playlist, update the time stamp for the last item



    m_root->currentIndex = playlistIndex;
    playlist->currentIndex = itemIndex;
    emit currentIndexChanged();
    return playInfo;
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
    tryPlay(m_root->currentIndex, newIndex);
}

void PlaylistManager::onLocalDirectoryChanged(const QString &path) {
    int index = m_root->indexOf(path);
    if (index == -1)  return;
    qInfo() << "Log (Playlist)   : Path" << path << "changed" << index;
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
        qWarning() << "Failed to reload folder" << m_root->at(index)->link;
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
    MpvObject::instance()->showText (QByteArrayLiteral("Setting subtitle: ") + url.toEncoded());
    MpvObject::instance()->addSubtitle(url);
    MpvObject::instance()->setSubVisible(true);
}

bool PlaylistManager::registerPlaylist(PlaylistItem *playlist) {
    if (!playlist || playlistSet.contains (playlist->link)) return false;
    playlist->use();
    playlistSet.insert(playlist->link);

    // Watch playlist path if local folder
    if (playlist->isLoadedFromFolder()) {
        m_folderWatcher.addPath(playlist->link);
    }
    return true;
}

void PlaylistManager::unregisterPlaylist(PlaylistItem *playlist) {
    if (!playlist || !playlistSet.contains (playlist->link)) return;
    playlistSet.remove(playlist->link);
    // Unwatch playlist path if local folder
    if (playlist->isLoadedFromFolder()) {
        m_folderWatcher.removePath(playlist->link);
    }
    playlist->disuse();
}

void PlaylistManager::appendPlaylist(PlaylistItem *playlist) {
    if (!registerPlaylist (playlist)) return;
    auto row = m_root->size();
    beginInsertRows(QModelIndex(), row, row);
    m_root->append(playlist);
    endInsertRows();
}

void PlaylistManager::replaceMainPlaylist(PlaylistItem *playlist) {
    // Main playlist is the first playlist
    static bool createdMainPlaylist = false;
    if (m_root->isEmpty()) {
        createdMainPlaylist = true;
        appendPlaylist(playlist); // root is empty so we append the playlist
    } else if (createdMainPlaylist && m_root->at (0)->link != playlist->link) {
        replacePlaylistAt(0, playlist);
    } else if (!createdMainPlaylist){
        createdMainPlaylist = true;
        registerPlaylist(playlist);
        beginInsertRows(QModelIndex(), 0, 0);
        m_root->insert(0, playlist);
        endInsertRows();
    }
}

void PlaylistManager::replacePlaylistAt(int index, PlaylistItem *newPlaylist) {
    auto playlistToReplace = m_root->at(index);
    if (!newPlaylist || !playlistToReplace) return;
    registerPlaylist(newPlaylist);
    unregisterPlaylist(playlistToReplace);

    beginRemoveRows(QModelIndex(), index, index);
    endRemoveRows();
    beginInsertRows(QModelIndex(), index, index);
    m_root->replace(index, newPlaylist);
    endInsertRows();

    qDebug() << "Log (Playlist)   : Replaced index" << index
             << "with" << newPlaylist->link;
}

void PlaylistManager::openUrl(const QUrl &url, bool playUrl) {
    if (!url.isValid()) return;
    QString urlString = url.toString();

    if (url.isLocalFile()) {
        PlaylistItem *playlist = PlaylistItem::fromLocalUrl(url);
        replaceMainPlaylist(playlist);
        if (playUrl) tryPlay (0, -1);
        return;
    }
    if (!m_client.isOk(urlString)) {
        return;
    }
    // Get the playlist for pasted online videos
    auto pastePlaylistIndex = m_root->indexOf ("videos");
    PlaylistItem *pastePlaylist = nullptr;
    if (pastePlaylistIndex == -1) {
        // Create a new one
        // qDebug() << "created videos";
        pastePlaylist = new PlaylistItem("Videos", nullptr, "videos");
        pastePlaylistIndex = m_root->size();
        appendPlaylist(pastePlaylist);
    } else {
        // Set the index to that index
        pastePlaylist = m_root->getCurrentItem();
        qDebug() << "found videos " << pastePlaylistIndex;
        // int row = playlist->size();

    }
    if (pastePlaylist->indexOf (urlString) != -1) return;

    auto parent = createIndex(pastePlaylistIndex, 0, pastePlaylist);
    beginInsertRows(parent, pastePlaylist->size(), pastePlaylist->size());
    pastePlaylist->emplaceBack (0, pastePlaylist->size() + 1, urlString, urlString, true);
    endInsertRows();

    if (playUrl) {
        tryPlay (pastePlaylistIndex, pastePlaylist->size() - 1);
    }

}

void PlaylistManager::pasteOpen(QString path) {
    if (path.isEmpty()) {
        path = QGuiApplication::clipboard()->text().trimmed();
        if ((path.startsWith('\'') && path.endsWith('\'')) ||
            (path.startsWith('"') && path.endsWith('"'))
            ) {
            path.removeAt(0);
            path.removeLast();
        }
        path.replace("\\/", "/");
        qDebug() << "Pasting" << path;
    }

    // static QRegularExpression urlPattern(R"(https?:\/\/(?:www\.|(?!www))[a-zA-Z0-9][a-zA-Z0-9-]+[a-zA-Z0-9]\.[^\s]{2,}|www\.[a-zA-Z0-9][a-zA-Z0-9-]+[a-zA-Z0-9]\.[^\s]{2,}|https?:\/\/(?:www\.|(?!www))[a-zA-Z0-9]+\.[^\s]{2,}|www\.[a-zA-Z0-9]+\.[^\s]{2,})");
    // QRegularExpressionMatch match = urlPattern.match(path);
    // check if online url
    // if (path.startsWith("http") && !m_client.isOk(path))
        // return;

    QUrl url = QUrl::fromUserInput(path);


    if (m_subtitleExtensions.contains(QFileInfo(url.path()).suffix())) {
        setSubtitle(url);
    } else if ( MpvObject::instance()->getCurrentVideoUrl() != url){
        if (url.isLocalFile()) {
            appendFolderPlaylist(url);
        } else {
            openUrl(url, true);
        }
        MpvObject::instance()->showText(QByteArrayLiteral("Playing: ") + path.toUtf8());


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

void PlaylistManager::loadServer(int index) {
    static QFutureWatcher<void> serverLoadWatcher;
    if (serverLoadWatcher.isRunning()) {
        m_isCancelled = true;
        return;
    }

    setIsLoading(true);
    serverLoadWatcher.setFuture(QtConcurrent::run([&](){
        auto playInfo = m_serverListModel.extract(&m_client, index);
        auto currentPlaylist = m_root->getCurrentItem();
        if (m_isCancelled) return;
        if (!playInfo.sources.isEmpty()) {
            m_serverListModel.setCurrentIndex(index);
            auto serverName = m_serverListModel.getServerAt(index).name;
            currentPlaylist->getProvider()->setPreferredServer(serverName);
            qInfo() << "Log (Server): Fetched source" << playInfo.sources.first().videoUrl;
            if (m_isCancelled) return;
            MpvObject::instance()->open (playInfo.sources.first(), MpvObject::instance()->time());
            if (!playInfo.subtitles.isEmpty())
                m_subtitleListModel.setList(playInfo.subtitles);

        } else {
            m_serverListModel.removeServerAt(index);
        }
        setIsLoading(false);
    }));


}

QString PlaylistManager::getCurrentItemName() const {
    auto currentPlaylist = m_root->getCurrentItem();
    if (!currentPlaylist) return "";
    return currentPlaylist->getDisplayNameAt (currentPlaylist->currentIndex);
}

QModelIndex PlaylistManager::getCurrentIndex() const {
    PlaylistItem *currentPlaylist = m_root->getCurrentItem();
    if (!currentPlaylist ||
        !currentPlaylist->isValidIndex (currentPlaylist->currentIndex))
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
    case IndexInParentRole:
        if (!item->parent()) return -1;
        return item->parent()->indexOf(item);
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
        {IndexInParentRole, "indexInParent"}
    };
    return names;
}
