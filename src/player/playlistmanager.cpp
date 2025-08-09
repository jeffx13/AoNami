#include "playlistmanager.h"
#include "utils/logger.h"
#include "utils/myexception.h"
#include "player/mpvObject.h"
#include "utils/errorhandler.h"
#include "providers/showprovider.h"
#include <QtConcurrent/QtConcurrentRun>

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

    m_watcher.setFuture(QtConcurrent::run(&PlaylistManager::play, this, playlistIndex, itemIndex));
    return true;

}

PlayItem PlaylistManager::play(int playlistIndex, int itemIndex) {

    auto playlist = m_root->at(playlistIndex);
    auto episode = playlist->at(itemIndex);

    PlayItem playItem;

    if (episode->type == PlaylistItem::PASTED) {
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

        m_serverListModel.clear();
    }
    else if (episode->type == PlaylistItem::LOCAL) {
        if (!QDir(playlist->link).exists()) {
            beginResetModel();
            unregisterPlaylist(playlist);
            m_root->removeOne(playlist);
            m_root->setCurrentIndex(m_root->isEmpty() ? -1 : 0);
            endResetModel();
            return playItem;
        }

        if(playlist->getCurrentIndex() != itemIndex){
            playlist->setCurrentIndex(itemIndex);
            playlist->updateHistoryFile();
        }
        playItem.videos.emplaceBack(episode->link);
        m_serverListModel.clear();
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

    m_root->setCurrentIndex(playlistIndex);
    playlist->setCurrentIndex(itemIndex);
    emit currentIndexChanged();
    return playItem;
}

void PlaylistManager::openUrl(QUrl url, bool play) {
    QString urlString;

    // if URL is empty, try to get it from clipboard
    if (url.isEmpty()) {
        urlString = QGuiApplication::clipboard()->text().trimmed();

        // check if curl
        static QRegularExpression urlRegex(R"(curl\s+'([^']+)')");
        QRegularExpressionMatch urlMatch = urlRegex.match(urlString);
        if (urlMatch.hasMatch()) {
            QString delimiter = "|";
            QStringList parts;
            parts << urlMatch.captured(1);;
            // Extract headers
            static QRegularExpression headerRegex(R"(-H\s+'([^']+)')");
            QRegularExpressionMatchIterator it = headerRegex.globalMatch(urlString);

            while (it.hasNext()) {
                QRegularExpressionMatch match = it.next();
                QString header = match.captured(1);
                parts << header;
            }
            urlString = parts.join(delimiter);
            url = QUrl::fromUserInput(urlMatch.captured(1));
        }
        else if ((urlString.startsWith('\'') && urlString.endsWith('\'')) ||
                   (urlString.startsWith('"') && urlString.endsWith('"')))
        {
            urlString.removeAt(0);
            urlString.removeLast();
            urlString.replace("\\/", "/");
            url = QUrl::fromUserInput(urlString);
        }
    }


    // if (!url.isValid()) {
    //     ErrorHandler::instance().show("Invalid url:" + urlString, "Player");
    //     return;
    // }

    static QStringList m_subtitleExtensions = { "srt", "sub", "ssa", "ass", "idx", "vtt" };
    if (m_subtitleExtensions.contains(QFileInfo(url.path()).suffix()) || url.path().toLower().contains("subtitle") ) {
        // int subtitleIndex = m_subtitleListModel.addSubtitle(url);
        MpvObject::instance()->addSubtitle(Track(url));
        // loadSubtitle(subtitleIndex);
        return;
    }

    // static QRegularExpression urlPattern(R"(https?:\/\/(?:www\.|(?!www))[a-zA-Z0-9][a-zA-Z0-9-]+[a-zA-Z0-9]\.[^\s]{2,}|www\.[a-zA-Z0-9][a-zA-Z0-9-]+[a-zA-Z0-9]\.[^\s]{2,}|https?:\/\/(?:www\.|(?!www))[a-zA-Z0-9]+\.[^\s]{2,}|www\.[a-zA-Z0-9]+\.[^\s]{2,})");
    int playlistIndex = -1;
    if (url.isLocalFile()) {
        cLog() << "Playlist" << "Opening local file" << url;
        playlistIndex = append(fromLocalUrl(url));
    } else { // Online video
        cLog() << "Playlist" << "Opening online video" << urlString;

        playlistIndex = m_root->indexOf("videos");
        if (playlistIndex == -1) {
            // Create a playlist for pasted videos
            playlistIndex = append(new PlaylistItem("Videos", nullptr, "videos"));
        }

        PlaylistItem *pastePlaylist = m_root->at(playlistIndex);
        auto itemIndex = pastePlaylist->indexOf(urlString);
        if (itemIndex == -1) {
            auto parent = createIndex(playlistIndex, 0, pastePlaylist);
            beginInsertRows(parent, pastePlaylist->size(), pastePlaylist->size());
            pastePlaylist->emplaceBack(0, pastePlaylist->size() + 1, urlString, urlString, true);
            pastePlaylist->last();

            pastePlaylist->last()->type = PlaylistItem::PASTED;
            itemIndex = pastePlaylist->size() - 1;
        }
        pastePlaylist->setCurrentIndex(itemIndex);
    }

    if (play && playlistIndex != -1 && MpvObject::instance()->getCurrentVideoUrl() != url) {
        MpvObject::instance()->showText(QString("Playing: %1").arg(urlString.toUtf8()));
        tryPlay(playlistIndex);
    }

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

    if (!loadFromFolder(playlist, QUrl())) {
        // Folder is empty, deleted, can't open history file etc.
        unregisterPlaylist(playlist);
        beginResetModel();
        m_root->removeAt(index);
        endResetModel();
        m_root->setCurrentIndex(m_root->isEmpty() ? -1 : 0);
        emit currentIndexChanged();
        cLog() << "Playlist" << "Failed to reload folder" << m_root->at(index)->link;
    }

    if (isCurrent) {
        QString newLink = playlist->getCurrentItem()->link;
        if (prevlink != newLink) {
            tryPlay();
        }
    }
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
            // throw MyException(QString("Failed to load server %1").arg(serverName), "Server");
            oLog() << "Server" << QString("Failed to load server %1").arg(serverName);
            return playItem;
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

void PlaylistManager::loadOffset(int playlistOffset, int itemOffset) {

    auto playlistIndex = m_root->getCurrentIndex() + playlistOffset;
    if (m_root->getCurrentIndex() == -1 || !m_root->isValidIndex(playlistIndex)) return;

    auto newPlaylist = m_root->at(playlistIndex);

    auto currentItemIndex = newPlaylist->getCurrentIndex();
    int newItemIndex = currentItemIndex + itemOffset;

    if (newItemIndex == newPlaylist->size() && playlistIndex + 1 < m_root->size()) {
        // Play next playlist
        auto nextPlaylist = m_root->at(playlistIndex + 1);
        newItemIndex = nextPlaylist->getCurrentIndex() == -1 ? 0 : nextPlaylist->getCurrentIndex();
        playlistIndex = playlistIndex + 1;
    } else if (newItemIndex < 0 && playlistIndex - 1 >= 0) {
        // Play previous playlist
        auto prevPlaylist = m_root->at(m_root->getCurrentIndex() - 1);
        newItemIndex = prevPlaylist->getCurrentIndex() == -1 ? 0 : prevPlaylist->getCurrentIndex();
        playlistIndex = playlistIndex - 1;
    }

    if (!newPlaylist->isValidIndex(newItemIndex)) return;

    tryPlay(playlistIndex, newItemIndex);
}

void PlaylistManager::reload() {
    auto currentPlaylist = m_root->getCurrentItem();
    if (!currentPlaylist || !currentPlaylist->getCurrentItem()) return;
    auto time = MpvObject::instance()->time();
    currentPlaylist->getCurrentItem()->timeStamp = time;
    tryPlay();
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

}

int PlaylistManager::append(PlaylistItem *playlist) {
    if (!playlist) return -1;
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

    unregisterPlaylist(playlistToReplace);
    beginRemoveRows(QModelIndex(), index, index);
    m_root->removeAt(index);
    endRemoveRows();
    beginInsertRows(QModelIndex(), index, index);
    m_root->insert(index, newPlaylist);
    endInsertRows();
    return index;
}

void PlaylistManager::removeAt(int index) {
    auto currentPlaylistIndex = m_root->getCurrentIndex();
    if (!m_root->isValidIndex(index) || index == m_root->getCurrentIndex()) {
        return;
    }

    PlaylistItem *playlistToRemove = m_root->at(index);

    // Begin removal operation
    beginRemoveRows(QModelIndex(), index, index);
    unregisterPlaylist(playlistToRemove);
    m_root->removeAt(index);
    endRemoveRows();

    // Correct the current playlist index
    if (index < currentPlaylistIndex) {
        m_root->setCurrentIndex(currentPlaylistIndex - 1);
        emit currentIndexChanged();
    }
}

void PlaylistManager::removeByModelIndex(QModelIndex index) {
    auto playlist = ((PlaylistItem*)(index.constInternalPointer()));
    if (playlist->parent() != m_root) return;
    int i = m_root->indexOf(playlist);
    removeAt(i);
}

void PlaylistManager::clear() {
    beginRemoveRows(QModelIndex(), 0, m_root->size() - 1);
    auto currentPlaylist = m_root->getCurrentItem();
    for (int i = 0; i < m_root->size(); i++) {
        const auto &playlist = m_root->at(i);
        if (playlist == currentPlaylist) continue;
        unregisterPlaylist(playlist);
        m_root->removeOne(playlist);
    }
    endRemoveRows();
    beginInsertRows(QModelIndex(), 0, 0);
    endInsertRows();
    m_root->setCurrentIndex(m_root->isEmpty() ? -1 : 0);
    emit currentIndexChanged();

}

void PlaylistManager::setIsLoading(bool value) {
    m_isLoading = value;
    emit isLoadingChanged();
}

void PlaylistManager::showCurrentItemName() const {
    auto currentPlaylist = m_root->getCurrentItem();
    if (!currentPlaylist) return;
    auto itemName = currentPlaylist->getDisplayNameAt(currentPlaylist->getCurrentIndex());
    MpvObject::instance()->showText(itemName);
}

PlaylistItem *PlaylistManager::fromLocalUrl(const QUrl &pathUrl) {
    if (!pathUrl.isLocalFile())
        return nullptr;

    PlaylistItem *playlist = new PlaylistItem("", nullptr, "");
    playlist->m_isLoadedFromFolder = true;
    playlist->m_children = std::unique_ptr<QList<PlaylistItem*>>(new QList<PlaylistItem*>);

    if (loadFromFolder(playlist, pathUrl)) {
        delete playlist;
        return nullptr;
    }

    return playlist;
}

bool PlaylistManager::loadFromFolder(PlaylistItem *playlist, const QUrl &pathUrl) {
    if (!playlist->isLoadedFromFolder()) return false;
    clear();

    QDir playlistDir;
    QString openedFilename;

    if (!pathUrl.isEmpty()) {
        QFileInfo path = QFileInfo(pathUrl.toLocalFile());
        if (!path.exists()) {
            oLog() << "Playlist" << path.absoluteFilePath() << "doesn't exist";
            return false;
        }

        if (!path.isDir()) {
            playlistDir = path.dir();
            openedFilename = path.fileName();
        } else {
            playlistDir = QDir(pathUrl.toLocalFile());
        }
        playlist->m_historyFile = std::make_unique<QFile> (playlistDir.filePath(".mpv.history"));
        playlist->name = playlistDir.dirName();
        playlist->fullName = playlistDir.dirName();
        playlist->link = playlistDir.absolutePath();

    } else {
        if (playlist->link.isEmpty()) return false;
        playlistDir = QDir(playlist->link);
        if (!playlistDir.exists()) {
            oLog() << "Playlist" << playlist->link << "doesn't exist";
            playlist->currentIndex = -1;
            return false;
        }
    }

    QStringList fileNames = playlistDir.entryList(
        {"*.mp4", "*.mkv", "*.avi", "*.mp3", "*.flac", "*.wav", "*.ogg", "*.webm", "*.m3u8", "*.mov"}, QDir::Files);
    if (fileNames.isEmpty()) {
        oLog() << "Playlist" << "No files to play in" << playlistDir.absolutePath();
        playlist->currentIndex = -1;
        return false;
    }

    QString lastPlayedFile = "";
    QString timeString = "";

    // Read history file
    if (playlist->m_historyFile->exists()) {
        // Open history file
        bool fileOpened = playlist->m_historyFile->isOpen() ? true : playlist->m_historyFile->open(QIODevice::ReadOnly | QIODevice::Text);
        if (!fileOpened) {
            rLog() << "Playlist" << "Failed to open history file";
            playlist->currentIndex = -1;
            return false;
        }
        auto fileData = QTextStream(playlist->m_historyFile.get()).readAll().trimmed().split(":");
        playlist->m_historyFile->close();
        if (!fileData.isEmpty()) {
            lastPlayedFile = fileData.first();
            if (fileData.size() == 2) {
                timeString = fileData.last();
            }
        }
    }

    // Check if the opened file is different from the last played file
    if (!openedFilename.isEmpty() && lastPlayedFile != openedFilename) {
        bool fileOpened = playlist->m_historyFile->isOpen() ? true : playlist->m_historyFile->open(QIODevice::WriteOnly | QIODevice::Text);
        if (!fileOpened) {
            rLog() << "Playlist" << "Failed to open history file";
            return false;
        }
        playlist->m_historyFile->write(openedFilename.toUtf8());
        playlist->m_historyFile->close();
        timeString.clear();
        lastPlayedFile = openedFilename;
    }

    PlaylistItem *currentItemPtr = nullptr;
    static QRegularExpression fileNameRegex{ R"((?:Episode|Ep\.?)?\s*(?<number>\d+\.?\d*)?\s*[\.:]?\s*(?<title>.*)?\.\w{3})" };

    for (const auto &fileName: std::as_const(fileNames)) {
        QRegularExpressionMatch match = fileNameRegex.match(fileName);
        QString title = match.hasMatch() ? match.captured("title").trimmed() : "";
        float itemNumber = (match.hasMatch() && !match.captured("number").isEmpty()) ? match.captured("number").toFloat() : -1;
        playlist->emplaceBack (0, itemNumber,  playlistDir.absoluteFilePath(fileName), title, true);
        if (fileName == lastPlayedFile) {
            // Set current item
            currentItemPtr = playlist->m_children->last();
        }
    }


    // sort the episodes in order
    std::stable_sort(playlist->m_children->begin(), playlist->m_children->end(),
                     [](const PlaylistItem *a, const PlaylistItem *b) {
                         return a->number < b->number;
                     });

    if (currentItemPtr) {
        playlist->currentIndex = playlist->indexOf(currentItemPtr);
        if (!timeString.isEmpty()) {
            bool ok;
            int intTime = timeString.toInt (&ok);
            if (ok) {
                currentItemPtr->timeStamp = intTime;
            }
        }
    }

    if (playlist->currentIndex < 0) playlist->currentIndex = 0;
    return true;
}

QModelIndex PlaylistManager::getCurrentIndex(QModelIndex i) const {
    auto currentPlaylist = static_cast<PlaylistItem *>(i.internalPointer());
    if (!currentPlaylist ||
        !currentPlaylist->isValidIndex(currentPlaylist->getCurrentIndex()))
        return QModelIndex();
    return index(currentPlaylist->getCurrentIndex(), 0, index(m_root->indexOf(currentPlaylist), 0, QModelIndex()));
}

QModelIndex PlaylistManager::getCurrentModelIndex() const {
    PlaylistItem *currentPlaylist = m_root->getCurrentItem();
    if (!currentPlaylist ||
        !currentPlaylist->isValidIndex(currentPlaylist->getCurrentIndex()))
        return QModelIndex();

    return index(currentPlaylist->getCurrentIndex(), 0, index(m_root->getCurrentIndex(), 0, QModelIndex()));
}

QModelIndex PlaylistManager::getCurrentListIndex() {
    return createIndex(m_root->getCurrentIndex(), 0, m_root->getCurrentItem());
}

// QAbstractItemModel functions

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
        if (!item->parent() || item->parent()->getCurrentIndex() == -1) return false;
        return item->parent()->getCurrentItem() == item;
        break;
    }
    default:
        break;
    }
    return QVariant();
}

QHash<int, QByteArray> PlaylistManager::roleNames() const {
    QHash<int, QByteArray> names
        {
            {TitleRole, "title"},
            {NumberRole, "number"},
            {IndexRole, "index"},
            {NumberTitleRole, "numberTitle"},
            {IsCurrentIndexRole, "isCurrentIndex"}
        };
    return names;
}

