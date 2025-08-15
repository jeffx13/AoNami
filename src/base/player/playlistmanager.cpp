#include "playlistmanager.h"
#include "app/logger.h"
#include "app/myexception.h"
#include "base/player/mpvObject.h"
#include "gui/errordisplayer.h"
#include <QtConcurrent/QtConcurrentRun>

PlaylistManager::PlaylistManager(QObject *parent) : ServiceManager(parent)
{
    registerPlaylist(m_root);
    connect (&m_folderWatcher, &QFileSystemWatcher::directoryChanged, this, &PlaylistManager::onLocalDirectoryChanged);
    connect (&m_watcher, &QFutureWatcher<PlayInfo>::finished, this, &PlaylistManager::onLoadFinished);
    connect (&m_watcher, &QFutureWatcher<PlayInfo>::started, this, [this](){
        m_isCancelled = false;
        setIsLoading(true);
    });

}

void PlaylistManager::saveProgress() const {
    // Update the last play time
    if (!m_currentItem) return;
    auto playlist = m_currentItem->parent(); // m_currentItem can never be a LIST
    if (!playlist || playlist->type != PlaylistItem::LIST) return;
    auto row = m_currentItem->row();
    auto timestamp = MpvObject::instance()->time();
    cLog() << "Playlist" << playlist->name << "Saving | Index =" << row << "| Timestamp =" << timestamp;

    bool isLastItem = row != playlist->count() - 1;
    bool almostFinished = timestamp > (0.95 * MpvObject::instance()->duration());

    // Prevents playing the next item immediately when coming back to this index again
    if (isLastItem && almostFinished) timestamp = 0;

    playlist->getCurrentItem()->setTimestamp(timestamp);
    playlist->updateHistoryFile();
    emit progressUpdated(playlist->link, row, timestamp);
}

void PlaylistManager::onLoadFinished() {
    if (!m_isCancelled.load()) {
        try {
            m_currentPlayItem = m_watcher.result();

            if (!m_currentPlayItem.videos.isEmpty()) {
                // sort videos
                std::sort(m_currentPlayItem.videos.begin(), m_currentPlayItem.videos.end(),
                          [](const Video &a, const Video &b) {
                              if (a.resolution > b.resolution) return true;
                              if (a.resolution < b.resolution) return false;
                              return a.bitrate > b.bitrate;
                          });

                MpvObject::instance()->open(m_currentPlayItem);
                emit aboutToPlay();
            }
        } catch (MyException& ex) {
            ex.show();
        } catch(const std::runtime_error& ex) {
            ErrorDisplayer::instance().show (ex.what(), "Playlist Error");
        } catch (...) {
            ErrorDisplayer::instance().show ("Something went wrong", "Playlist Error");
        }
    }
    setIsLoading(false);
    m_isCancelled = false;
}

bool PlaylistManager::tryPlay(int playlistIndex, int itemIndex) {
    playlistIndex = playlistIndex == -1 ? (m_root->getCurrentIndex() == -1 ? 0 : m_root->getCurrentIndex()) : playlistIndex;
    auto playlist = m_root->at(playlistIndex);
    if (!playlist) return false;

    // Set to current playlist item index if -1
    itemIndex = itemIndex == -1 ? (playlist->getCurrentIndex() == -1 ? 0 : playlist->getCurrentIndex()) : itemIndex;
    return tryPlay(playlist->at(itemIndex));
}

bool PlaylistManager::tryPlay(PlaylistItem *item) {
    if (!item) return false;
    auto link = item->type == PlaylistItem::LIST ? item->link : (item->parent() ? item->parent()->link : "");
    if (!m_playlistMap.contains(link)) {
        rLog() << "Playlist" << "Item" << item->name << "is not registered";
        return false;
    }

    if (m_watcher.isRunning()) {
        m_isCancelled = true;
        return false;
    }
    m_isCancelled = false;
    setIsLoading(true);
    saveProgress();
    m_watcher.setFuture(QtConcurrent::run(&PlaylistManager::play, this, item));
    return true;
}

void PlaylistManager::loadNextItem(int offset) {
    if (!m_currentItem) {
        tryPlay(m_root->at(0));
        return;
    }
    auto playlist = m_currentItem->parent();
    if (!playlist) {
        rLog() << "Playlist" << m_currentItem->name << "does not belong to a playlist";
        return;
    }

    auto nextItemIndex = m_currentItem->row() + offset;
    auto parentPlaylist = playlist->parent();
    auto playlistIndex = playlist->row();
    PlaylistItem *nextItem = nullptr;

    if (parentPlaylist) {
        bool playlistChanged = false;
        if (nextItemIndex == playlist->count() && playlistIndex + 1 < parentPlaylist->count()) {
            // Play next playlist
            playlist = parentPlaylist->at(playlistIndex + 1);
            playlistChanged = true;
        } else if (nextItemIndex < 0 && playlistIndex - 1 >= 0) {
            // Play previous playlist
            playlist = parentPlaylist->at(playlistIndex - 1);
            playlistChanged = true;
        }
        if (playlistChanged)
            nextItemIndex = playlist->getCurrentIndex() == -1 ? 0 : playlist->getCurrentIndex();
    }
    nextItem = playlist->at(nextItemIndex);
    tryPlay(nextItem); // nullptr handled if nextItemIndex is out of bounds
}

void PlaylistManager::loadNextPlaylist(int offset) {
    if (!m_currentItem || !m_currentItem->parent()) {
        tryPlay(m_root->at(0));
        return;
    }
    auto playlist = m_currentItem->parent();
    auto parentPlaylist = playlist->parent();
    if (!parentPlaylist) return;
    auto row = playlist->row();

    int nextPlaylistIndex = row + offset;
    tryPlay(parentPlaylist->at(nextPlaylistIndex));
}

void PlaylistManager::loadServer(int index) {
    if (m_watcher.isRunning()) {
        m_isCancelled = true;
        return;
    }
    if (!m_serverListModel.isValidIndex(index)) return;

    m_watcher.setFuture(QtConcurrent::run([&, index](){
        auto client = Client(&m_isCancelled);
        auto serverName = m_serverListModel.at(index).name;
        PlayInfo playItem = m_serverListModel.loadServer(&client, index);
        if (playItem.videos.isEmpty()) {
            oLog() << "Server" << QString("Failed to load server %1").arg(serverName);
            return playItem;
        }
        playItem.timeStamp = MpvObject::instance()->time();
        m_serverListModel.setCurrentIndex(index);
        m_serverListModel.setPreferredServer(index);

        return playItem;
    }));
}

void PlaylistManager::loadIndex(const QModelIndex &index) {
    tryPlay(static_cast<PlaylistItem *>(index.internalPointer()));
}



void PlaylistManager::reload() {
    if (!m_currentItem) return;
    auto time = MpvObject::instance()->time();
    m_currentItem->setTimestamp(time);
    tryPlay(m_currentItem);
}

void PlaylistManager::registerPlaylist(PlaylistItem *playlist) {
    if (!playlist || !playlist->isList() || m_playlistMap.contains(playlist->link)) return;

    playlist->use();
    m_playlistMap.insert(playlist->link, playlist);

    // Watch playlist path if local folder
    if (playlist->isLocalDir()) {
        m_folderWatcher.addPath(playlist->link);
    }
}

void PlaylistManager::deregisterPlaylist(PlaylistItem *playlist) {
    if (!playlist || !m_playlistMap.contains(playlist->link)) return;
    m_playlistMap.remove(playlist->link);
    // Unwatch playlist path if local folder
    if (playlist->isLocalDir()) {
        m_folderWatcher.removePath(playlist->link);
    }

}








void PlaylistManager::remove(QModelIndex modelIndex) {
    auto item = static_cast<PlaylistItem*>(modelIndex.internalPointer());
    auto parent = item->parent();
    auto row = modelIndex.row();
    if (!parent || (parent->getCurrentIndex() != -1 && parent->getCurrentItem() == item)) return;

    if (item->isList()) deregisterPlaylist(item);
    emit aboutToRemove(item);
    parent->removeAt(row);
    emit removed();
    if (!m_currentItem) return;
    setCurrentItem(m_currentItem); // Updates the current indices
    emit updateSelections(m_currentItem);
}

void PlaylistManager::clear() {
    auto currentPlaylist = m_root->getCurrentItem();
    auto it = m_root->iterator();
    while (it.hasNext()) {
        auto playlist = it.next();
        if (playlist == currentPlaylist) continue;
        deregisterPlaylist(playlist);
        m_root->removeOne(playlist);
    }

    emit modelReset();
    m_root->setCurrentIndex(currentPlaylist ? 0 : -1);
    emit updateSelections(m_currentItem);
}

PlaylistItem *PlaylistManager::find(const QString &link) {
    return m_playlistMap.value(link, nullptr);
}

int PlaylistManager::insert(int index, PlaylistItem *playlist, PlaylistItem *parent) {
    if (!playlist) return -1;
    parent = parent == nullptr ? m_root : parent;
    if (!parent->isList()) return -1;
    if (m_playlistMap.contains(playlist->link)) {
        return m_playlistMap.value(playlist->link)->row();
    }

    registerPlaylist(playlist);
    index = index < 0 ? 0 : (index >= parent->count() ? parent->count() : index);

    emit aboutToInsert(parent, index);
    parent->insert(index, playlist);
    emit inserted();
    return index;
}

int PlaylistManager::replace(int index, PlaylistItem *playlist, PlaylistItem *parent) {
    if (!playlist) return -1;
    if (m_playlistMap.contains(playlist->link))
        return m_playlistMap.value(playlist->link)->row();

    parent = parent == nullptr ? m_root : parent;
    if (!parent->isList()) return -1;
    if (!parent->isValidIndex(index)) {
        rLog() << "Playlist" << "Invalid index:" << index << "to replace";
        return -1;
    }
    deregisterPlaylist(parent->at(index));
    registerPlaylist(playlist);
    if (index == parent->getCurrentIndex())
        saveProgress();

    parent->removeAt(index);
    parent->insert(index, playlist);
    emit changed(index);
    return index;
}



void PlaylistManager::showCurrentItemName() const {
    auto currentPlaylist = m_root->getCurrentItem();
    if (!currentPlaylist) return;
    auto currentItem = currentPlaylist->getCurrentItem();
    if (!currentItem) return;
    QString itemName = "%1\n[%2/%3] %4";
    itemName = itemName.arg(currentPlaylist->name)
                   .arg(currentPlaylist->getCurrentIndex() + 1)
                   .arg(currentPlaylist->count())
                   .arg(currentItem->displayName);

    MpvObject::instance()->showText(itemName);
}

void PlaylistManager::openUrl(QUrl url, bool play) {
    QString urlString;

    // if URL is empty, retrieve the text from the clipboard
    if (url.isEmpty()) {
        auto clipboard = QGuiApplication::clipboard()->text().trimmed();

        // Check if curl
        if (clipboard.startsWith("curl")) {
            static QRegularExpression curlRegex(R"(curl\s+'([^']+)')");
            QRegularExpressionMatch urlMatch = curlRegex.match(clipboard);
            if (urlMatch.hasMatch()) {
                QString delimiter = "|";
                QStringList parts;
                parts << urlMatch.captured(1);;
                // Extract headers
                static QRegularExpression headerRegex(R"(-H\s+'([^']+)')");
                QRegularExpressionMatchIterator it = headerRegex.globalMatch(clipboard);

                while (it.hasNext()) {
                    QRegularExpressionMatch match = it.next();
                    QString header = match.captured(1);
                    parts << header;
                }
                urlString = parts.join(delimiter);
                url = QUrl::fromUserInput(urlMatch.captured(1));
            }
        } else {
            if (clipboard.startsWith("\""))
                clipboard.remove(0, 1);
            if (clipboard.endsWith("\""))
                clipboard.chop(1);

            url = QUrl::fromUserInput(clipboard);
            urlString = clipboard;
        }
    }

    if (!url.isValid()) {
        rLog() << "Playlist" << "Invalid url:" << urlString;
        return;
    }

    static QStringList m_subtitleExtensions = { "srt", "sub", "ssa", "ass", "idx", "vtt" };
    if (m_subtitleExtensions.contains(QFileInfo(url.path()).suffix()) || url.path().toLower().contains("subtitle") ) {
        MpvObject::instance()->addSubtitle(Track(url));
        return;
    }

    // static QRegularExpression urlPattern(R"(https?:\/\/(?:www\.|(?!www))[a-zA-Z0-9][a-zA-Z0-9-]+[a-zA-Z0-9]\.[^\s]{2,}|www\.[a-zA-Z0-9][a-zA-Z0-9-]+[a-zA-Z0-9]\.[^\s]{2,}|https?:\/\/(?:www\.|(?!www))[a-zA-Z0-9]+\.[^\s]{2,}|www\.[a-zA-Z0-9]+\.[^\s]{2,})");
    int playlistIndex = -1;
    if (url.isLocalFile()) {
        cLog() << "Playlist" << "Opening local file" << url;

        // Check if dir has already been added
        auto pathInfo = QFileInfo(url.toLocalFile());
        auto dirPath = pathInfo.dir().absolutePath();
        if (m_playlistMap.contains(dirPath)) {
            playlistIndex = m_root->indexOf(dirPath);
            // If url is a filepath then update the index to item to play
            if (!pathInfo.isDir()) {
                auto playlist = m_root->at(playlistIndex);
                m_root->at(playlistIndex)->setCurrentIndex(playlist->indexOf(pathInfo.absoluteFilePath()));
            }
        } else {
            auto playlist = loadFromFolder(url);
            if (playlist) {
                playlistIndex = append(playlist);
            }
        }
    } else { // Online video
        cLog() << "Playlist" << "Opening online video" << urlString;

        playlistIndex = m_root->indexOf("videos");
        if (playlistIndex == -1) {
            // Create a playlist for pasted videos
            auto playlist = new PlaylistItem("Videos", nullptr, "videos");
            playlistIndex = append(playlist);
        }

        PlaylistItem *pastePlaylist = m_root->at(playlistIndex);
        auto itemIndex = pastePlaylist->indexOf(urlString);
        if (itemIndex == -1) {
            emit aboutToInsert(pastePlaylist, pastePlaylist->count());
            pastePlaylist->emplaceBack(0, pastePlaylist->count() + 1, urlString, url.toString(), true);
            emit inserted();
            pastePlaylist->last()->type = PlaylistItem::PASTED;
            itemIndex = pastePlaylist->count() - 1;
        }
        pastePlaylist->setCurrentIndex(itemIndex);
    }

    if (play && playlistIndex != -1 && MpvObject::instance()->getCurrentVideoUrl() != url) {
        MpvObject::instance()->showText(QString("Playing: %1").arg(urlString.toUtf8()));
        tryPlay(playlistIndex);
    }

}


void PlaylistManager::cancel() {
    if (!m_watcher.isRunning()) return;
    qDebug() << "cancelling";
    m_isCancelled = true;
}

PlayInfo PlaylistManager::play(PlaylistItem *item) {
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

void PlaylistManager::setCurrentItem(PlaylistItem *currentItem) {
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

PlaylistItem *PlaylistManager::loadFromFolder(const QUrl &pathUrl, PlaylistItem *playlist) {
    bool createdNewPlaylist = false;
    if (!playlist) {
        if (pathUrl.isEmpty()) return nullptr;
        playlist = new PlaylistItem("", nullptr, "");
        playlist->setIsLocalDir(true);
        createdNewPlaylist = true;
    }

    if (!playlist->isLocalDir())
        return playlist;

    auto url = !pathUrl.isEmpty() ? pathUrl : QUrl::fromUserInput(playlist->link);

    if (!url.isValid()) {
        if (createdNewPlaylist) {
            delete playlist;
            playlist = nullptr;
        }
        return playlist;
    }
    QFileInfo pathInfo = QFileInfo(pathUrl.toLocalFile());
    QDir playlistDir;

    if (!pathInfo.exists()) {
        oLog() << "Playlist" << pathInfo.absoluteFilePath() << "doesn't exist";
        if (createdNewPlaylist) {
            delete playlist;
            playlist = nullptr;
        }
        return playlist;
    }

    if (!pathInfo.isDir()) {
        playlistDir = pathInfo.dir();
    } else {
        playlistDir = QDir(pathUrl.toLocalFile());
    }

    QStringList playableFiles = playlistDir.entryList({"*.mp4", "*.mkv", "*.avi", "*.mp3", "*.flac", "*.wav", "*.ogg", "*.webm", "*.m3u8", "*.mov"}, QDir::Files);

    // Delete the playlist if no playable files
    if (playableFiles.isEmpty()) {
        oLog() << "Playlist" << "No files to play in" << playlistDir.absolutePath();
        if (playlist->parent() == m_root){
            int index = m_root->indexOf(playlist);
            if (index != -1 && m_root->getCurrentIndex() == index) {
                m_root->setCurrentIndex(-1);
                m_root->removeAt(index);
            }
        }
        return nullptr;
    }


    playlist->name = playlistDir.dirName();
    playlist->displayName = playlistDir.dirName();
    playlist->link = playlistDir.absolutePath();
    playlist->m_historyFile = std::make_unique<QFile>(playlistDir.filePath(".mpv.history"));

    QString fileToPlay = "";
    QString timestamp = "";

    // If pathUrl is a directory, attempt to read history file for last played file
    if (pathInfo.isDir()) {
        if (playlist->m_historyFile->exists()) {
            bool fileOpened = playlist->m_historyFile->isOpen() ? true : playlist->m_historyFile->open(QIODevice::ReadOnly | QIODevice::Text);
            if (fileOpened) {
                auto fileData = QTextStream(playlist->m_historyFile.get()).readAll().trimmed().split(":");
                playlist->m_historyFile->close();
                if (!fileData.isEmpty()) {
                    fileToPlay = fileData.first();
                    if (fileData.size() == 2) {
                        timestamp = fileData.last();
                    }
                }
            } else {
                rLog() << "Playlist" << "Failed to open history file";
            }
        }
    } else if (playableFiles.contains(pathInfo.fileName())){
        // Update the histroy if pathUrl is a filepath
        bool fileOpened = playlist->m_historyFile->isOpen() ? true : playlist->m_historyFile->open(QIODevice::WriteOnly | QIODevice::Text);
        if (fileOpened) {
            playlist->m_historyFile->write(pathInfo.fileName().toUtf8());
            playlist->m_historyFile->close();
            fileToPlay = pathInfo.fileName();
        }
        else {
            rLog() << "Playlist" << "Failed to open and update history file";
        }
    }

    // Keep track of the pointer to the last played file
    PlaylistItem *currentItemPtr = nullptr;
    static QRegularExpression fileNameRegex{ R"((?:Episode|Ep\.?)?\s*(?<number>\d+\.?\d*)?\s*[\.:]?\s*(?<title>.*)?\.\w{3})" };

    for (const auto &file: std::as_const(playableFiles)) {
        QRegularExpressionMatch match = fileNameRegex.match(file);
        QString title = match.hasMatch() ? match.captured("title").trimmed() : "";
        float itemNumber = (match.hasMatch() && !match.captured("number").isEmpty()) ? match.captured("number").toFloat() : -1;
        playlist->emplaceBack(0, itemNumber,  playlistDir.absoluteFilePath(file), title, true);
        if (file == fileToPlay) {
            // Set current item
            currentItemPtr = playlist->m_children->last();
        }
    }


    // Sort the episodes in order
    std::stable_sort(playlist->m_children->begin(), playlist->m_children->end(),
                     [](const PlaylistItem *a, const PlaylistItem *b) {
                         return a->number < b->number;
                     });

    if (currentItemPtr) {
        playlist->setCurrentIndex(playlist->indexOf(currentItemPtr));
        if (!timestamp.isEmpty()) {
            bool ok;
            int intTime = timestamp.toInt(&ok);
            if (ok) {
                currentItemPtr->setTimestamp(intTime);
            }
        }
    }

    if (playlist->getCurrentIndex() < 0)
        playlist->setCurrentIndex(0);
    return playlist;
}

void PlaylistManager::onLocalDirectoryChanged(const QString &path) {
    auto playlist = m_playlistMap.value(path, nullptr);
    if (!playlist)  {
        rLog() << "Playlist" << "Untracked path" << path;
        return;
    }
    auto currentItem = playlist->getCurrentItem();
    QString prevlink = currentItem == m_currentItem ? currentItem->link : "";

    // Try reloading the folder
    if (!loadFromFolder(QUrl(), playlist)) {
        // Folder is empty, deleted, can't open history file etc.
        cLog() << "Playlist" << "Failed to reload folder" << playlist->link;
        deregisterPlaylist(playlist);
        auto parent = playlist->parent();
        Q_ASSERT(parent);
        auto row = playlist->row();
        parent->removeOne(playlist);
        emit modelReset();
        if (parent->getCurrentIndex() == row) {
            parent->setCurrentIndex(m_root->isEmpty() ? -1 : 0);
            MpvObject::instance()->pause();
            // TODO Pause
        } else if (parent->getCurrentIndex() > row) {
            parent->setCurrentIndex(parent->getCurrentIndex() - 1);
        }
        setCurrentItem(nullptr);
        updateSelections(nullptr, false);
        return;
    }

    currentItem = playlist->getCurrentItem();
    if (!prevlink.isEmpty() && currentItem) {
        QString newLink = currentItem->link; // item may have been deleted so playlist might set index to 0
        if (prevlink != newLink) {
            tryPlay(currentItem);
        }
    }
}

