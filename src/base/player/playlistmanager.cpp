#include "playlistmanager.h"
#include "app/logger.h"
#include "app/appexception.h"
#include "base/player/mpvObject.h"
#include "gui/uibridge.h"
#include <QtConcurrent/QtConcurrentRun>

PlaylistManager::PlaylistManager(QObject *parent)
    : ServiceManager(parent)
{
    registerPlaylist(m_root.get());
    connect(&m_folderWatcher, &QFileSystemWatcher::directoryChanged, this, &PlaylistManager::onLocalDirectoryChanged);
    connect(&m_watcher, &QFutureWatcher<PlayInfo>::finished, this, &PlaylistManager::onLoadFinished);
    connect(&m_watcher, &QFutureWatcher<PlayInfo>::started, this, [this]() {
        m_isCancelled = false;
        setIsLoading(true);
    });
}

bool PlaylistManager::tryPlay(int playlistIndex, int itemIndex) {
    if (playlistIndex == -1)
        playlistIndex = (m_root->getCurrentIndex() == -1) ? 0 : m_root->getCurrentIndex();
    auto playlist = m_root->at(playlistIndex);
    if (!playlist) return false;

    if (itemIndex == -1)
        itemIndex = (playlist->getCurrentIndex() == -1) ? 0 : playlist->getCurrentIndex();
    return tryPlay(playlist->at(itemIndex));
}

bool PlaylistManager::tryPlay(PlaylistItem *item) {
    if (!item) return false;
    QString link = item->isList() ? item->link : (item->parent() ? item->parent()->link : "");
    PlaylistItem *playlist = m_playlistMap.value(link, nullptr);
    if (!playlist) {
        rLog() << "Playlist" << (item->isList() ? item->name : item->parent()->name) << "is not registered";
        return false;
    }
    if (!item->isList() && item->parent() != playlist) {
        oLog() << "Playlist" << "Item does not belong to registered playlist";
        int itemIndex = playlist->indexOf(item->link);
        if (itemIndex != -1) {
            item = playlist->at(itemIndex);
        } else {
            rLog() << "Item does not belong to registered playlist";
            return false;
        }
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

PlayInfo PlaylistManager::play(PlaylistItem *item) {
    if (!item) return {};
    PlaylistItem *playlist = nullptr;

    if (item->isList()) {
        if (item->isEmpty()) return {};
        auto currentItem = item->getCurrentItem();
        playlist = item;
        item = currentItem ? currentItem : item->at(0);
    } else {
        playlist = item->parent();
    }

    if (!playlist || !playlist->isList()) {
        rLog() << "Playlist" << item->name << "does not belong to any playlist!";
        return {};
    }

    PlayInfo playInfo;
    m_serverListModel.clear();
    int itemRow = item->row();

    switch (item->type) {
    case PlaylistItem::PASTED: {
        if (item->link.contains('|')) {
            QStringList parts = item->link.split('|');
            playInfo.videos.emplaceBack(parts.takeFirst());
            for (const QString &headerLine : parts) {
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
            throw AppException("Cannot get provider from playlist!", "Provider");

        QString episodeName = item->displayName.trimmed().replace('\n', " ");
        auto servers = provider->loadServers(&m_client, item);
        if (servers.isEmpty())
            throw AppException("No servers found for " + episodeName, "Server");

        std::sort(servers.begin(), servers.end(),
                  [](const VideoServer &a, const VideoServer &b) {
                      return a.name < b.name;
                  });

        auto result = ServerListModel::findWorkingServer(&m_client, provider, servers);
        if (result.first == -1)
            throw AppException("No working server found for " + episodeName, "Server");

        if (m_isCancelled) return {};
        m_serverListModel.setServers(servers, provider);
        m_serverListModel.setCurrentIndex(result.first);
        m_serverListModel.setPreferredServer(result.first);
        playInfo = result.second;
        break;
    }
    case PlaylistItem::LOCAL: {
        if (!QFile::exists(item->link)) {
            oLog() << "Playlist" << item->link << "does not exist";
            aboutToRemove(item);
            playlist->removeAt(itemRow);
            emit removed();
            if (playlist->getCurrentItem() == item)
                playlist->setCurrentIndex(-1);
            return {};
        }
        playInfo.videos.emplaceBack(item->link);
        break;
    }
    case PlaylistItem::LIST:
        return {};
    }

    if (m_isCancelled) return {};
    if (playlist->getCurrentIndex() != itemRow) {
        playlist->setCurrentIndex(itemRow);
        playlist->updateHistoryFile();
    }
    auto parent = playlist;
    int row = itemRow;
    while (parent) {
        parent->setCurrentIndex(row);
        row = parent->row();
        parent = parent->parent();
    }
    playInfo.timestamp = item->getTimestamp();

    emit progressUpdated(playlist->link, itemRow, item->getTimestamp());
    m_currentItem = item;
    emit updateSelections(m_currentItem, true);
    return playInfo;
}

void PlaylistManager::openUrl(QUrl url, bool play) {
    QString urlString;

    if (url.isEmpty()) {
        QString clipboard = QGuiApplication::clipboard()->text().trimmed();

        if (clipboard.startsWith("curl")) {
            static QRegularExpression curlRegex(R"(curl\s+'([^']+)')");
            QRegularExpressionMatch urlMatch = curlRegex.match(clipboard);
            if (urlMatch.hasMatch()) {
                QString delimiter = "|";
                QStringList parts;
                parts << urlMatch.captured(1);
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
    if (m_subtitleExtensions.contains(QFileInfo(url.path()).suffix()) || url.path().toLower().contains("subtitle")) {
        MpvObject::instance()->addSubtitle(Track(url));
        return;
    }

    PlaylistItem *playlist = nullptr;
    if (url.isLocalFile()) {
        QFileInfo pathInfo(url.toLocalFile());
        QString dirPath = pathInfo.isDir() ? pathInfo.absoluteFilePath() : pathInfo.dir().absolutePath();
        cLog() << "Playlist" << "Opening local file" << dirPath;

        if (m_playlistMap.contains(dirPath)) {
            playlist = m_playlistMap.value(dirPath);
            if (!pathInfo.isDir()) {
                playlist->setCurrentIndex(playlist->indexOf(pathInfo.absoluteFilePath()));
            }
        } else {
            playlist = new PlaylistItem;
            if (loadFromFolder(url, playlist, true)) {
                append(playlist);
                cLog() << "Playlist" << "Loaded folder" << dirPath;
            } else {
                cLog() << "Playlist" << "Failed to load folder" << dirPath;
                delete playlist;
                playlist = nullptr;
            }
        }
    } else {
        cLog() << "Playlist" << "Opening online video" << urlString;
        playlist = m_playlistMap.value("videos", nullptr);
        if (!playlist) {
            playlist = new PlaylistItem("Videos", nullptr, "videos");
            append(playlist);
        }

        int itemIndex = playlist->indexOf(urlString);
        if (itemIndex == -1) {
            emit aboutToInsert(playlist, playlist->count());
            playlist->emplaceBack(0, playlist->count() + 1, urlString, url.toString(), true);
            emit inserted();
            playlist->last()->type = PlaylistItem::PASTED;
            itemIndex = playlist->count() - 1;
        }
        playlist->setCurrentIndex(itemIndex);
    }
    if (playlist && play) {
        MpvObject::instance()->showText(QString("Playing: %1").arg(urlString.toUtf8()));
        tryPlay(playlist);
    }
}

void PlaylistManager::onLoadFinished() {
    if (!m_isCancelled.load()) {
        try {
            m_currentPlayItem = m_watcher.result();

            if (!m_currentPlayItem.videos.isEmpty()) {
                std::sort(m_currentPlayItem.videos.begin(), m_currentPlayItem.videos.end(),
                          [](const Video &a, const Video &b) {
                              if (a.resolution > b.resolution) return true;
                              if (a.resolution < b.resolution) return false;
                              return a.bitrate > b.bitrate;
                          });

                MpvObject::instance()->open(m_currentPlayItem);
                emit aboutToPlay();
            }
        } catch (AppException &ex) {
            ex.show();
        } catch (const std::runtime_error &ex) {
            UiBridge::instance().showError(ex.what(), "Playlist Error");
        } catch (...) {
            UiBridge::instance().showError("Something went wrong", "Playlist Error");
        }
    }
    setIsLoading(false);
    m_isCancelled = false;
}

void PlaylistManager::reload() {
    if (!m_currentItem) return;
    int time = MpvObject::instance()->time();
    m_currentItem->setTimestamp(time);
    tryPlay(m_currentItem);
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

    int nextItemIndex = m_currentItem->row() + offset;
    auto parentPlaylist = playlist->parent();
    int playlistIndex = playlist->row();

    if (parentPlaylist) {
        bool playlistChanged = false;
        if (nextItemIndex == playlist->count() && playlistIndex + 1 < parentPlaylist->count()) {
            playlist = parentPlaylist->at(playlistIndex + 1);
            playlistChanged = true;
        } else if (nextItemIndex < 0 && playlistIndex - 1 >= 0) {
            playlist = parentPlaylist->at(playlistIndex - 1);
            playlistChanged = true;
        }
        if (playlistChanged)
            nextItemIndex = (playlist->getCurrentIndex() == -1) ? 0 : playlist->getCurrentIndex();
    }
    PlaylistItem *nextItem = playlist->at(nextItemIndex);
    tryPlay(nextItem);
}

void PlaylistManager::loadNextPlaylist(int offset) {
    if (!m_currentItem || !m_currentItem->parent()) {
        tryPlay(m_root->at(0));
        return;
    }
    auto playlist = m_currentItem->parent();
    auto parentPlaylist = playlist->parent();
    if (!parentPlaylist) return;
    int row = playlist->row();

    int nextPlaylistIndex = row + offset;
    tryPlay(parentPlaylist->at(nextPlaylistIndex));
}

void PlaylistManager::loadServer(int index) {
    if (m_watcher.isRunning()) {
        m_isCancelled = true;
        return;
    }
    if (!m_serverListModel.isValidIndex(index)) return;

    m_watcher.setFuture(QtConcurrent::run([&, index]() {
        Client client(&m_isCancelled);
        QString serverName = m_serverListModel.at(index).name;
        PlayInfo playItem = m_serverListModel.loadServer(&client, index);
        if (playItem.videos.isEmpty()) {
            oLog() << "Server" << QString("Failed to load server %1").arg(serverName);
            return playItem;
        }
        playItem.timestamp = MpvObject::instance()->time();
        m_serverListModel.setCurrentIndex(index);
        m_serverListModel.setPreferredServer(index);

        return playItem;
    }));
}

void PlaylistManager::saveProgress() const {
    if (!m_currentItem) return;
    auto playlist = m_currentItem->parent();
    if (!playlist || playlist->type != PlaylistItem::LIST) return;
    int row = m_currentItem->row();
    int timestamp = MpvObject::instance()->time();
    cLog() << "Playlist" << playlist->name << "Saving | Index =" << row << "| Timestamp =" << timestamp;

    bool isLastItem = row == playlist->count() - 1;
    bool almostFinished = timestamp > (0.95 * MpvObject::instance()->duration());

    if (isLastItem && almostFinished) timestamp = 0;

    playlist->getCurrentItem()->setTimestamp(timestamp);
    playlist->updateHistoryFile();
    emit progressUpdated(playlist->link, row, timestamp);
}

void PlaylistManager::registerPlaylist(PlaylistItem *playlist) {
    if (!playlist || !playlist->isList() || m_playlistMap.contains(playlist->link)) return;
    QList<PlaylistItem *> items{playlist};
    while (!items.isEmpty()) {
        auto item = items.takeFirst();
        m_playlistMap.insert(item->link, item);
        if (item->isLocalDir()) {
            m_folderWatcher.addPath(item->link);
        }
        auto it = item->iterator();
        while (it.hasNext()) {
            auto child = it.next();
            if (!child->isList()) continue;
            items.append(child);
        }
    }
}

void PlaylistManager::deregisterPlaylist(PlaylistItem *playlist) {
    if (!playlist) return;
    if (!m_playlistMap.contains(playlist->link)) {
        rLog() << "Playlist" << "Attempting to deregister unregistered playlist" << playlist->name;
        return;
    }

    QList<PlaylistItem *> items{playlist};
    while (!items.isEmpty()) {
        auto item = items.takeFirst();
        m_playlistMap.remove(item->link);
        if (item->isLocalDir()) {
            m_folderWatcher.removePath(item->link);
        }
        auto it = item->iterator();
        while (it.hasNext()) {
            auto child = it.next();
            if (!child->isList()) continue;
            items.append(child);
        }
    }
}

void PlaylistManager::remove(QModelIndex modelIndex) {
    auto item = static_cast<PlaylistItem *>(modelIndex.internalPointer());
    auto parent = item->parent();
    int row = modelIndex.row();
    if (!parent || (parent->getCurrentIndex() != -1 && parent->getCurrentItem() == item)) return;

    if (item->isList()) {
        deregisterPlaylist(item);
    }
    emit aboutToRemove(item);
    parent->removeAt(row);
    emit removed();

    if (parent->isEmpty()) {
        auto grandparent = parent->parent();
        if (grandparent) {
            emit aboutToRemove(parent);
            grandparent->removeAt(parent->row());
            emit removed();
        }
    }

    if (!m_currentItem) return;
    setCurrentItem(m_currentItem);
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

int PlaylistManager::insert(int index, PlaylistItem *playlist, PlaylistItem *parent) {
    if (!playlist) return -1;
    if (!parent) parent = m_root.get();
    if (!parent->isList()) return -1;
    if (m_playlistMap.contains(playlist->link)) {
        return m_playlistMap.value(playlist->link)->row();
    }

    registerPlaylist(playlist);
    index = (index < 0) ? 0 : (index >= parent->count() ? parent->count() : index);

    emit aboutToInsert(parent, index);
    parent->insert(index, playlist);
    emit inserted();
    return index;
}

int PlaylistManager::replace(int index, PlaylistItem *playlist, PlaylistItem *parent) {
    if (!playlist) return -1;
    if (m_playlistMap.contains(playlist->link))
        return m_playlistMap.value(playlist->link)->row();

    if (!parent) parent = m_root.get();
    if (!parent->isList()) return -1;
    if (!parent->isValidIndex(index)) {
        rLog() << "Playlist" << "Invalid index:" << index << "to replace";
        return -1;
    }
    deregisterPlaylist(parent->at(index));
    registerPlaylist(playlist);
    if (index == parent->getCurrentIndex())
        saveProgress();
    emit aboutToRemove(parent->at(index));
    parent->removeAt(index);
    emit removed();
    emit aboutToInsert(parent, index);
    parent->insert(index, playlist);
    emit inserted();
    return index;
}

void PlaylistManager::showCurrentItemName() const {
    if (!m_currentItem) return;
    auto playlist = m_currentItem->parent();
    if (!playlist) return;
    QString path = playlist->name;
    auto current = playlist->parent();
    while (current && current != m_root.get()) {
        path = current->name + " | " + path;
        current = current->parent();
    }

    QString displayText = QString("%1\n[%2/%3] %4")
                              .arg(path)
                              .arg(playlist->getCurrentIndex() + 1)
                              .arg(playlist->count())
                              .arg(m_currentItem->displayName.replace("\n", " "));

    MpvObject::instance()->showText(displayText);
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

bool PlaylistManager::loadFromFolder(const QUrl &pathUrl, PlaylistItem *playlist, bool recursive) {
    QUrl url = !pathUrl.isEmpty() ? pathUrl : QUrl::fromUserInput(playlist->link);
    if (!url.isValid() || !url.isLocalFile()) return false;
    QFileInfo pathInfo(url.toLocalFile());
    if (!pathInfo.exists()) {
        oLog() << "Playlist" << pathInfo.absoluteFilePath() << "doesn't exist";
        return false;
    }
    QDir playlistDir = pathInfo.isDir() ? QDir(url.toLocalFile()) : pathInfo.dir();
    QFileInfoList playableFiles = playlistDir.entryInfoList(m_playableExtensions, QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot);

    playlist->name = playlistDir.dirName();
    playlist->displayName = playlistDir.dirName();
    playlist->link = playlistDir.absolutePath();
    playlist->setIsLocalDir(true);
    playlist->clear();

    if (playableFiles.isEmpty()) return false;

    playlist->m_historyFile = std::make_unique<QFile>(playlistDir.filePath(".mpv.history"));

    QString fileToPlay;
    int timestamp = 0;

    if (pathInfo.isDir() || pathInfo.completeSuffix() == "mpv.history") {
        if (playlist->m_historyFile->exists()) {
            bool fileOpened = playlist->m_historyFile->isOpen() ? true : playlist->m_historyFile->open(QIODevice::ReadOnly | QIODevice::Text);
            if (fileOpened) {
                auto fileData = QTextStream(playlist->m_historyFile.get()).readAll().trimmed().split(":");
                playlist->m_historyFile->close();
                if (!fileData.isEmpty()) {
                    fileToPlay = fileData.first();
                    if (fileData.size() == 2) {
                        timestamp = fileData.last().toInt();
                    }
                }
            } else {
                rLog() << "Playlist" << "Failed to open history file";
            }
        }
    } else if (playableFiles.contains(pathInfo)) {
        bool fileOpened = playlist->m_historyFile->isOpen() ? true : playlist->m_historyFile->open(QIODevice::WriteOnly | QIODevice::Text);
        if (fileOpened) {
            playlist->m_historyFile->write(pathInfo.fileName().toUtf8());
            playlist->m_historyFile->close();
            fileToPlay = pathInfo.fileName();
        } else {
            rLog() << "Playlist" << "Failed to open and update history file";
        }
    }

    PlaylistItem *currentItemPtr = nullptr;
    static QRegularExpression fileNameRegex{R"((?:[Ss](?<S>\d{1,2})[Ee](?<E>\d{1,3})[\s\-\.]*| (?<episode>\d{2,3}) ?[\s\-]*)(?<title>[^\(\)]+\w)?.*?\.\w{3,4}$)"};
    for (int i = 0; i < playableFiles.count(); ++i) {
        const QFileInfo &fileInfo = playableFiles[i];
        QString path = fileInfo.absoluteFilePath();
        if (fileInfo.isFile()) {
            QRegularExpressionMatch match = fileNameRegex.match(fileInfo.fileName());
            QString title;
            int season = 0;
            float episodeNumber = -1;
            if (match.hasMatch()) {
                title = match.hasCaptured("title") ? match.captured("title").trimmed() : "";
                season = match.hasCaptured("S") ? match.captured("S").trimmed().toInt() : 0;
                QString episodeStr = match.hasCaptured("E") ? match.captured("E").trimmed() : (match.hasCaptured("episode") ? match.captured("episode") : "");
                bool ok;
                float ep = episodeStr.toFloat(&ok);
                episodeNumber = ok ? ep : i;
            } else {
                title = fileInfo.fileName();
            }
            playlist->emplaceBack(season, episodeNumber, path, title, true);

            if (fileInfo.fileName() == fileToPlay) {
                currentItemPtr = playlist->m_children->last();
            }
        } else if (fileInfo.isDir() && recursive) {
            if (m_playlistMap.contains(path)) continue;
            auto subPlaylist = new PlaylistItem;
            if (loadFromFolder(QUrl::fromLocalFile(path), subPlaylist) && !subPlaylist->isEmpty()) {
                playlist->append(subPlaylist);
            } else {
                delete subPlaylist;
            }
        }
    }
    if (playlist->isEmpty()) return false;

    std::stable_sort(playlist->m_children->begin(), playlist->m_children->end(),
                     [](const PlaylistItem *a, const PlaylistItem *b) {
                         if (a->isList() && !b->isList()) return true;
                         if (!a->isList() && b->isList()) return false;
                         if (!a->isList() && !b->isList()) {
                             if (a->seasonNumber == b->seasonNumber) return a->number < b->number;
                             return a->seasonNumber < b->seasonNumber;
                         }
                         return a->name < b->name;
                     });

    if (currentItemPtr) {
        playlist->setCurrentIndex(playlist->indexOf(currentItemPtr));
        currentItemPtr->setTimestamp(timestamp);
    }
    return true;
}

void PlaylistManager::onLocalDirectoryChanged(const QString &path) {
    auto playlist = m_playlistMap.value(path, nullptr);
    if (!playlist) {
        rLog() << "Playlist" << "Untracked path" << path;
        return;
    }
    auto currentItem = playlist->getCurrentItem();
    QString prevlink = (currentItem && currentItem == m_currentItem) ? currentItem->link : "";
    QDir playlistDir(playlist->link);
    Q_ASSERT(playlistDir.exists());
    QFileInfoList playableFileList = playlistDir.entryInfoList(m_playableExtensions, QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot);

    QSet<QString> playableFileSet;
    QSet<QString> playlistItemSet;

    for (int i = 0; i < playableFileList.count(); ++i) {
        playableFileSet.insert(playableFileList[i].absoluteFilePath());
    }
    auto it = playlist->iterator();
    while (it.hasNext()) {
        playlistItemSet.insert(it.next()->link);
    }
    if (playableFileSet.intersect(playlistItemSet).count() == playlist->count())
        return;

    cLog() << "Playlist" << "Directory" << path << "has changed";
    if (loadFromFolder(playlist->link, playlist, true)) {
        currentItem = playlist->getCurrentItem();
        if (playlist == m_currentItem->parent() && !prevlink.isEmpty() && currentItem) {
            QString newLink = currentItem->link;
            if (prevlink != newLink) {
                tryPlay(currentItem);
            }
        }
        return;
    }
    cLog() << "Playlist" << "Failed to reload folder" << playlist->link;
    MpvObject::instance()->pause();
    if (playlist->getCurrentItem() == m_currentItem) {
        setCurrentItem(nullptr);
    }

    deregisterPlaylist(playlist);
    auto parent = playlist->parent();
    Q_ASSERT(parent);
    emit aboutToRemove(playlist);
    parent->removeOne(playlist);
    emit removed();

    updateSelections(m_currentItem, false);
}

void PlaylistManager::cancel() {
    if (!m_watcher.isRunning()) return;
    m_isCancelled = true;
}
