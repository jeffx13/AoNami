#include "playlistmanager.h"
#include "network/myexception.h"
#include "player/mpvObject.h"
#include "utils/errorhandler.h"
#include "providers/showprovider.h"

PlaylistManager::PlaylistManager(QObject *parent) {
    // Opens the file to play immediately when application launches

    connect (&m_folderWatcher, &QFileSystemWatcher::directoryChanged, this, &PlaylistManager::onLocalDirectoryChanged);

    connect (&m_watcher, &QFutureWatcher<void>::finished, this, [this](){
        if (!m_watcher.future().isValid()) {
            //future was cancelled
            ErrorHandler::instance().show ("Operation cancelled", "Error");
        }
        try {
            m_watcher.waitForFinished();
        } catch (MyException& ex) {
            ErrorHandler::instance().show (ex.what(), "Playlist Error");
        } catch(const std::runtime_error& ex) {
            ErrorHandler::instance().show (ex.what(), "Playlist Error");
        } catch (...) {
            ErrorHandler::instance().show ("Something went wrong", "Playlist Error");
        }
        m_shouldCancel = false;
        m_isLoading = false;
        emit isLoadingChanged();
    });


}

bool PlaylistManager::tryPlay(int playlistIndex, int itemIndex) {

    playlistIndex = playlistIndex == -1 ? (m_root->currentIndex == -1 ? 0 : m_root->currentIndex) : playlistIndex;
    auto newPlaylist = m_root->at (playlistIndex);
    if (!newPlaylist) return false;

    // Set to current playlist item index if -1
    itemIndex = itemIndex == -1 ? (newPlaylist->currentIndex == -1 ? 0 : newPlaylist->currentIndex) : itemIndex;
    if (!newPlaylist->isValidIndex (itemIndex) || newPlaylist->at (itemIndex)->type == PlaylistItem::LIST) {
        qWarning() << "Invalid index or attempting to play a list";
        return false;
    }


    if (m_watcher.isRunning()) {
        m_shouldCancel = true;
        m_watcher.waitForFinished ();
    }
    m_shouldCancel = false;
    m_watcher.setFuture(QtConcurrent::run(&PlaylistManager::play, this, playlistIndex, itemIndex));
    return true;

}

void PlaylistManager::play(int playlistIndex, int itemIndex) {
    m_isLoading = true;
    emit isLoadingChanged();

    auto playlist = m_root->at(playlistIndex);
    auto episode = playlist->at(itemIndex);
    PlayInfo playInfo;

    qDebug() << "Log (Playlist)   : Timestamp:" << playlist->at(itemIndex)->timeStamp;
    // QString episodeName = ;
    m_serverList.getSubtitleList()->clear();

    if (episode->type == PlaylistItem::LOCAL) {
        if (playlist->currentIndex != itemIndex){
            playlist->currentIndex = itemIndex;
            playlist->updateHistoryFile (0);
        }
        playInfo.sources.emplaceBack (episode->link);
    } else {
        ShowProvider *provider = playlist->getProvider();
        if (!provider){
            throw MyException("Cannot get provider from playlist!");
        }
        qInfo().noquote() << QString("Log (Playlist)   : Fetching servers for %1 [%2/%3]")
                                 .arg (episode->getFullName().trimmed()).arg (itemIndex + 1).arg (playlist->size());

        if (m_shouldCancel) return;
        QList<VideoServer> servers = provider->loadServers(episode);
        if (m_shouldCancel) return;

        if (servers.isEmpty()) {
            throw MyException("No servers found for " + episode->getFullName().trimmed());
        }
        qInfo().noquote() << "Log (Playlist)   : Successfully fetched servers for " << episode->getFullName().trimmed();

        playInfo = m_serverList.autoSelectServer(servers, provider);
        if (m_shouldCancel) return;
    }

    if (m_shouldCancel) return;
    // Update current item index only if videos are returned
    if (playInfo.sources.isEmpty()) {
        throw MyException("No sources extracted from " + episode->getFullName().trimmed());
        return;
    }

    qInfo() << "Log (Playlist)   : Fetched source" << playInfo.sources.first().videoUrl;



    MpvObject::instance()->open(playInfo.sources.first(), episode->timeStamp);

    // If same playlist, update thetime stamp for the last item
    auto currentPlaylist = m_root->getCurrentItem();
    if (currentPlaylist == playlist && currentPlaylist->currentIndex != -1 && currentPlaylist->currentIndex != itemIndex) {
        auto time = MpvObject::instance()->time();
        if (time > 0.85 * MpvObject::instance()->duration())
            time = 0;
        qInfo() << "Log (Playlist)   : Saving timestamp" << time << "for" << currentPlaylist->link;
        currentPlaylist->setLastPlayAt(currentPlaylist->currentIndex, time);
    }

    m_root->currentIndex = playlistIndex;
    playlist->currentIndex = itemIndex;
    emit aboutToPlay();
    emit currentIndexChanged();
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

    int index = m_root->indexOf (path);
    qInfo() << "Log (Playlist)   : Path" << path << "changed" << index;

    if (index > -1) {
        beginResetModel();
        QString prevlink = index == m_root->currentIndex ? m_root->getCurrentItem()->getCurrentItem()->link : "";
        if (!m_root->at (index)->reloadFromFolder()) {
            // Folder is empty, deleted, can't open history file etc.
            m_root->removeAt (index);
            m_root->currentIndex = m_root->isEmpty() ? -1 : 0;
            qWarning() << "Failed to reload folder" << m_root->at (index)->link;
        }
        endResetModel();
        emit currentIndexChanged();
        QString newLink = index == m_root->currentIndex ? m_root->getCurrentItem()->getCurrentItem()->link : "";
        if (prevlink != newLink) {
            tryPlay();
        }
    }
}

bool PlaylistManager::registerPlaylist(PlaylistItem *playlist) {
    if (!playlist || playlistSet.contains (playlist->link)) return false;

    playlistSet.insert(playlist->link);

    // Watch playlist path if local folder
    if (playlist->isLoadedFromFolder()) {
        m_folderWatcher.addPath (playlist->link);
    }
    return true;
}

void PlaylistManager::deregisterPlaylist(PlaylistItem *playlist) {
    if (!playlist || !playlistSet.contains (playlist->link)) return;
    playlistSet.remove(playlist->link);

    // Unwatch playlist path if local folder
    if (playlist->isLoadedFromFolder()) {
        m_folderWatcher.removePath (playlist->link);
    }
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
    if (m_root->isEmpty()) {
        appendPlaylist (playlist); // root is empty so we append the playlist
    }
    else if (m_root->at (0)->link != playlist->link) {
        replacePlaylistAt (0, playlist);
    }
}

void PlaylistManager::replacePlaylistAt(int index, PlaylistItem *newPlaylist) {
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

void PlaylistManager::openUrl(const QUrl &url, bool playUrl) {
    if (!url.isValid()) return;
    QString urlString = url.toString();

    if (url.isLocalFile()) {
        PlaylistItem *playlist = PlaylistItem::fromLocalUrl (url);
        replaceMainPlaylist(playlist);
        if (playUrl) tryPlay (0, -1);
        return;
    }

    // Get the playlist for pasted online videos
    auto pastePlaylistIndex = m_root->indexOf ("videos");
    PlaylistItem *pastePlaylist = nullptr;
    if (pastePlaylistIndex == -1) {
        // Create a new one
        qDebug() << "created videos";
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

    // Add the url to the playlist
    // auto parent = createIndex(0, 0, m_root);
    // beginInsertRows(index(pastePlaylistIndex, 0, parent), pastePlaylist->size(), pastePlaylist->size());
    // pastePlaylist->emplaceBack (pastePlaylist->size() + 1, urlString, urlString, true);
    // endInsertRows();

    auto parent = createIndex(pastePlaylistIndex, 0, pastePlaylist);
    beginInsertRows(parent, pastePlaylist->size(), pastePlaylist->size());
    pastePlaylist->emplaceBack (0, pastePlaylist->size() + 1, urlString, urlString, true);
    endInsertRows();


    if (playUrl) {
        tryPlay (pastePlaylistIndex, pastePlaylist->size() - 1);
    }

}

void PlaylistManager::pasteOpen() {
    QString clipboardText = QGuiApplication::clipboard()->text().trimmed();
    if ((clipboardText.startsWith("'") && clipboardText.endsWith("'")) ||
        (clipboardText.startsWith('"') && clipboardText.endsWith('"'))
        )
    {
        clipboardText.removeAt(0);
        clipboardText.removeLast();
    }
    QUrl url = QUrl::fromUserInput(clipboardText);
    if (!url.isValid()) return;
    QString extension = QFileInfo(url.path()).suffix();

    QStringList subtitleExtensions = {
        "srt", "sub", "ssa", "ass", "idx", "vtt",
    };
    if (subtitleExtensions.contains(extension)) {
        MpvObject::instance()->showText (QByteArrayLiteral("Setting Extension: ") + clipboardText.toUtf8());

        MpvObject::instance()->addSubtitle(url);
        MpvObject::instance()->setSubVisible(true);
    } else if (MpvObject::instance()->getCurrentVideoUrl() != url){

        static QRegularExpression urlPattern(R"(https?:\/\/(?:www\.|(?!www))[a-zA-Z0-9][a-zA-Z0-9-]+[a-zA-Z0-9]\.[^\s]{2,}|www\.[a-zA-Z0-9][a-zA-Z0-9-]+[a-zA-Z0-9]\.[^\s]{2,}|https?:\/\/(?:www\.|(?!www))[a-zA-Z0-9]+\.[^\s]{2,}|www\.[a-zA-Z0-9]+\.[^\s]{2,})");
        QRegularExpressionMatch match = urlPattern.match(clipboardText);
        if (!match.hasMatch())
            return;
        openUrl(url, true);
        MpvObject::instance()->showText (QByteArrayLiteral("Playing: ") + clipboardText.toUtf8());
    }

}

void PlaylistManager::setIsLoading(bool value) {
    m_isLoading = value;
    emit isLoadingChanged();
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
    case NumberRole:
        return item->number;
        break;
    case NumberTitleRole: {
        if (item->type == 0) {
            return item->name;
        }
        return item->getFullName();
        break;
    }
    default:
        break;
    }
    return QVariant();
}

QHash<int, QByteArray> PlaylistManager::roleNames() const {
    QHash<int, QByteArray> names;
    names[TitleRole] = "title";
    names[NumberRole] = "number";
    names[IndexRole] = "index";
    names[NumberTitleRole] = "numberTitle";
    return names;
}
