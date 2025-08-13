#include "playlistmanager.h"
#include "app/logger.h"
#include "app/myexception.h"
#include "base/player/mpvObject.h"
#include "gui/errordisplayer.h"
#include <QtConcurrent/QtConcurrentRun>

PlaylistManager::PlaylistManager(QObject *parent) : ServiceManager(parent)
{
    connect (&m_folderWatcher, &QFileSystemWatcher::directoryChanged, this, &PlaylistManager::onLocalDirectoryChanged);
    connect (&m_watcher, &QFutureWatcher<PlayItem>::finished, this, &PlaylistManager::onLoadFinished);
    connect (&m_watcher, &QFutureWatcher<PlayItem>::started, this, [this](){
        m_isCancelled = false;
        setIsLoading(true);
    });

}

void PlaylistManager::saveProgress() const {
    // Update the last play time
    auto playlist = m_root->getCurrentItem();
    if (!playlist || playlist->type != PlaylistItem::LIST || playlist->getCurrentIndex() == -1) return;
    auto currentIndex = playlist->getCurrentIndex();
    auto timestamp = MpvObject::instance()->time();
    cLog() << "Playlist" << playlist->name << "Saving | Index =" << currentIndex << "| Timestamp =" << timestamp;

    bool isLastItem = currentIndex != playlist->count() - 1;
    bool almostFinished = timestamp > (0.95 * MpvObject::instance()->duration());

    // Prevents playing the next item immediately when coming back to this index again
    if (isLastItem && almostFinished) timestamp = 0;

    playlist->getCurrentItem()->setTimestamp(timestamp);
    playlist->updateHistoryFile();
    emit progressUpdated(playlist->link, currentIndex, timestamp);
}

void PlaylistManager::updateSelection(bool scrollToIndex) {
    int playlistIndex = m_root->getCurrentIndex();
    int itemIndex = playlistIndex != -1 ? m_root->getCurrentItem()->getCurrentIndex() : -1;
    emit currentIndexChanged(playlistIndex, itemIndex, scrollToIndex);
}

void PlaylistManager::cancel() {
    if (m_watcher.isRunning()) {
        m_isCancelled = true;
    }
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
    // TODO connect to see if opened successfully as it might lose timestamp from switching servers
}

bool PlaylistManager::tryPlay(int playlistIndex, int itemIndex) {
    if (m_watcher.isRunning()) {
        m_isCancelled = true;
        return false;
    }

    playlistIndex = playlistIndex == -1 ? (m_root->getCurrentIndex() == -1 ? 0 : m_root->getCurrentIndex()) : playlistIndex;
    auto newPlaylist = m_root->at(playlistIndex);
    if (!newPlaylist) return false;

    // Set to current playlist item index if -1
    itemIndex = itemIndex == -1 ? (newPlaylist->getCurrentIndex() == -1 ? 0 : newPlaylist->getCurrentIndex()) : itemIndex;
    if (!newPlaylist->isValidIndex(itemIndex) || newPlaylist->at (itemIndex)->type == PlaylistItem::LIST) {
        oLog() << "Playlist" << "Invalid index or attempting to play a list";
        return false;
    }

    m_isCancelled = false;
    setIsLoading(true);

    saveProgress();

    m_watcher.setFuture(QtConcurrent::run(&PlaylistManager::play, this, playlistIndex, itemIndex));
    return true;

}

void PlaylistManager::loadNextItem(int offset) {
    auto playlistIndex = m_root->getCurrentIndex();
    if (playlistIndex == -1) {
        tryPlay(0);
        return;
    }
    auto playlist = m_root->getCurrentItem();
    auto itemIndex = playlist->getCurrentIndex() + offset; // Impossible for current item index to be -1

    if (itemIndex == playlist->count() && playlistIndex + 1 < m_root->count()) {
        // Play next playlist
        playlistIndex += 1;
        itemIndex = m_root->at(playlistIndex)->getCurrentIndex();
    } else if (itemIndex == -1 && playlistIndex - 1 >= 0) {
        // Play previous playlist
        playlistIndex -= 1;
        itemIndex = m_root->at(playlistIndex)->getCurrentIndex();
    } else if (!playlist->isValidIndex(itemIndex)) {
        return;
    }

    tryPlay(playlistIndex, itemIndex);

}

void PlaylistManager::loadNextPlaylist(int offset) {
    auto playlistIndex = m_root->getCurrentIndex();
    if (playlistIndex == -1) {
        tryPlay(0);
        return;
    }
    int nextPlaylistIndex = playlistIndex + offset;
    if (!m_root->isValidIndex(nextPlaylistIndex))
        return;

    tryPlay(nextPlaylistIndex);
}

PlayItem PlaylistManager::play(int playlistIndex, int itemIndex) {

    PlaylistItem *playlist = m_root->at(playlistIndex);
    PlaylistItem *episode = playlist->at(itemIndex);

    PlayItem playItem;
    m_serverListModel.clear();

    switch(episode->type) {
    case PlaylistItem::PASTED: {
        if (episode->link.contains('|')) {
            // curl command
            QStringList parts = episode->link.split('|');
            playItem.videos.emplaceBack(parts.takeFirst());
            for (const QString &headerLine : std::as_const(parts)) {
                QStringList keyValue = headerLine.split(": ", Qt::KeepEmptyParts);
                if (keyValue.size() == 2) {
                    playItem.headers.insert(keyValue[0].trimmed(), keyValue[1].trimmed());
                }
            }
        } else {
            playItem.videos.emplaceBack(episode->link);
        }
        break;
    }
    case PlaylistItem::ONLINE: {
        auto provider = playlist->getProvider();
        if (!provider)
            throw MyException("Cannot get provider from playlist!", "Provider");

        auto episodeName = episode->displayName.trimmed().replace('\n', " ");

        // Load server list
        auto servers = provider->loadServers(&m_client, episode);
        if (servers.isEmpty())
            throw MyException("No servers found for " + episodeName, "Server");

        // Sort servers by name
        std::sort(servers.begin(), servers.end(),
                  [](const VideoServer &a, const VideoServer &b) {
                      return a.name < b.name;
                  });

        // Find a working server
        auto result =
            ServerListModel::findWorkingServer(&m_client, provider, servers);
        if (result.first == -1)
            throw MyException("No working server found for " + episodeName, "Server");

        if (m_isCancelled.load())
            return {};
        // Set the servers and the index of the working server
        m_serverListModel.setServers(servers, provider);
        m_serverListModel.setCurrentIndex(result.first);
        m_serverListModel.setPreferredServer(result.first);
        playItem = result.second;
        break;
    }
    case PlaylistItem::LOCAL: {
        if (!QDir(playlist->link).exists()) {
            deregisterPlaylist(playlist);
            m_root->removeOne(playlist);
            m_root->setCurrentIndex(m_root->isEmpty() ? -1 : 0);
            emit modelReset();
            return playItem;
        }

        if (playlist->getCurrentIndex() != itemIndex) {
            playlist->setCurrentIndex(itemIndex);
            playlist->updateHistoryFile();
        }
        playItem.videos.emplaceBack(episode->link);
        break;
    }
    case PlaylistItem::LIST:
        break;
    }

    if (m_isCancelled) return {};

    playItem.timeStamp = episode->getTimestamp();

    m_root->setCurrentIndex(playlistIndex);
    playlist->setCurrentIndex(itemIndex);

    emit progressUpdated(playlist->link, itemIndex, playItem.timeStamp);
    updateSelection(true);
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
        auto serverName = m_serverListModel.at(index).name;
        PlayItem playItem = m_serverListModel.loadServer(&client, index);
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
    auto childItem = static_cast<PlaylistItem *>(index.internalPointer());
    auto parentItem = childItem->parent();
    if (parentItem == m_root) return;
    int itemIndex = childItem->row();
    int playlistIndex = m_root->indexOf(parentItem);

    tryPlay(playlistIndex, itemIndex);
}



void PlaylistManager::reload() {
    auto currentPlaylist = m_root->getCurrentItem();
    if (!currentPlaylist || !currentPlaylist->getCurrentItem()) return;
    auto time = MpvObject::instance()->time();
    currentPlaylist->getCurrentItem()->setTimestamp(time);
    tryPlay();
}

bool PlaylistManager::registerPlaylist(PlaylistItem *playlist) {
    if (!playlist || m_playlistSet.contains(playlist->link)) return false;
    playlist->use();
    m_playlistSet.insert(playlist->link);

    // Watch playlist path if local folder
    if (playlist->isLocalDir()) {
        m_folderWatcher.addPath(playlist->link);
    }
    return true;
}

void PlaylistManager::deregisterPlaylist(PlaylistItem *playlist) {
    if (!playlist || !m_playlistSet.contains(playlist->link)) return;
    m_playlistSet.remove(playlist->link);
    // Unwatch playlist path if local folder
    if (playlist->isLocalDir()) {
        m_folderWatcher.removePath(playlist->link);
    }

}

int PlaylistManager::append(PlaylistItem *playlist) {
    if (!playlist) return -1;
    if (!registerPlaylist(playlist)) {
        return m_root->indexOf(playlist->link);
    }
    auto row = m_root->count();
    emit aboutToInsert(m_root, row);
    m_root->append(playlist);
    emit inserted();
    return row;
}

int PlaylistManager::insert(int index, PlaylistItem *playlist) {
    if (index < 0 || index >= m_root->count()) {
        return append(playlist);
    }
    if (!registerPlaylist(playlist)) {
        return m_root->indexOf(playlist->link);
    }
    emit aboutToInsert(m_root, index);
    m_root->insert(index, playlist);
    emit inserted();
    return index;
}

int PlaylistManager::replace(int index, PlaylistItem *playlist) {
    if (!playlist) return -1;
    if (!m_root->isValidIndex(index)) return append(playlist);
    if (!registerPlaylist(playlist)) return m_root->indexOf(playlist->link);

    if (index == m_root->getCurrentIndex())
        saveProgress();

    auto playlistToReplace = m_root->at(index);

    deregisterPlaylist(playlistToReplace);
    m_root->removeAt(index);
    m_root->insert(index, playlist);
    emit changed(index);
    return index;
}


void PlaylistManager::remove(QModelIndex modelIndex) {
    auto playlist = static_cast<PlaylistItem*>(modelIndex.internalPointer());
    auto parent = playlist->parent();
    if (!parent || (parent->getCurrentIndex() != -1 && parent->at(parent->getCurrentIndex()) == playlist)) return;

    if (playlist->type == PlaylistItem::LIST) {
        deregisterPlaylist(playlist);
    }
    emit aboutToRemove(modelIndex);
    parent->removeAt(modelIndex.row());
    emit removed();
    if (modelIndex.row() < parent->getCurrentIndex()) {
        parent->setCurrentIndex(parent->getCurrentIndex() -1);
    }
    updateSelection();
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
    updateSelection();
}

PlaylistItem *PlaylistManager::find(const QString &link) {
    if (!m_playlistSet.contains(link)) return nullptr;
    return m_root->at(m_root->indexOf(link));
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
        if (m_playlistSet.contains(dirPath)) {
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
    int index = m_root->indexOf(path);
    if (index == -1)  return;
    auto playlist = m_root->at(index);

    QString prevlink;
    bool isCurrent = m_root->getCurrentIndex() == index && playlist->getCurrentIndex() != -1;
    if (isCurrent) {
        prevlink = playlist->getCurrentItem()->link;
    }

    if (!loadFromFolder(QUrl(), playlist)) {
        // Folder is empty, deleted, can't open history file etc.
        deregisterPlaylist(playlist);
        m_root->removeAt(index);
        emit modelReset();
        if (index == m_root->getCurrentIndex()) {
            m_root->setCurrentIndex(m_root->isEmpty() ? -1 : 0);

        } else if (index < m_root->getCurrentIndex()) {
            m_root->setCurrentIndex(m_root->getCurrentIndex() - 1);
        }
        updateSelection();
        cLog() << "Playlist" << "Failed to reload folder" << m_root->at(index)->link;
    }

    if (isCurrent) {
        QString newLink = playlist->getCurrentItem()->link;
        if (prevlink != newLink) {
            tryPlay();
        }
    }
}

