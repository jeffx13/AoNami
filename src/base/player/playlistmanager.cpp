#include "playlistmanager.h"
#include "app/logger.h"
#include "app/appexception.h"
#include "base/player/mpvObject.h"
#include "ui/uibridge.h"
#include <QtConcurrent/QtConcurrentRun>
#include <QDateTime>
#include <QGuiApplication>
#include <QClipboard>
#include <QRegularExpression>
#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <algorithm>
#include <QMetaObject>

PlaylistManager::PlaylistManager(QObject *parent) : ServiceManager(parent) {
    registerPlaylist(m_root);
    connect(&m_folderWatcher, &QFileSystemWatcher::directoryChanged, this, &PlaylistManager::onLocalDirectoryChanged);
    connect(&m_watcher, &QFutureWatcher<PlayInfo>::finished, this, [&]{
        if (!m_cancelled) {
            try {
                auto playItem = m_watcher.result();
                MpvObject::instance()->open(playItem);
            } catch (AppException &ex) {
                ex.show();
            } catch (const std::runtime_error &ex) {
                UiBridge::instance().showError(ex.what(), "Playlist Error");
            } catch (...) {
                UiBridge::instance().showError("Something went wrong", "Playlist Error");
            }
        }
        setIsLoading(false);
        m_cancelled = false;
    });
}

PlaylistManager::~PlaylistManager() {
    disconnect(&m_watcher, &QFutureWatcher<PlayInfo>::finished, this, nullptr);
    if (m_watcher.isRunning()) {
        m_cancelled = true;
        try { m_watcher.waitForFinished(); } catch(...) {}
    }
}

QSharedPointer<PlaylistItem> PlaylistManager::find(const QString &link) {
    auto it = m_playlistMap.find(link);
    return it != m_playlistMap.end() ? it.value().toStrongRef() : nullptr;
}

int PlaylistManager::append(QSharedPointer<PlaylistItem> playlist, QSharedPointer<PlaylistItem> parent) {
    return insert(INT_MAX, playlist, parent);
}

int PlaylistManager::insert(int index, QSharedPointer<PlaylistItem> playlist, QSharedPointer<PlaylistItem> parent) {
    if (!playlist) return -1;
    if (!parent) parent = m_root;
    if (!parent->isList()) return -1;
    auto existingPlaylist = m_playlistMap.value(playlist->link, QWeakPointer<PlaylistItem>()).toStrongRef();
    if (existingPlaylist)
        return existingPlaylist->row();

    registerPlaylist(playlist);
    index = (index < 0) ? 0 : (index >= parent->count() ? parent->count() : index);
    emit aboutToInsert(parent.data(), index);
    parent->insert(index, playlist);
    emit inserted();
    auto currentItem = m_currentItem.toStrongRef();
    setCurrentItem(currentItem);
    return index;
}

int PlaylistManager::replace(int index, QSharedPointer<PlaylistItem> playlist, QSharedPointer<PlaylistItem> parent) {
    if (!playlist) return -1;
    auto existingPlaylist = m_playlistMap.value(playlist->link, QWeakPointer<PlaylistItem>()).toStrongRef();
    if (existingPlaylist)
        return existingPlaylist->row();
    if (parent) {
        auto existingParent = m_playlistMap.value(parent->link, QWeakPointer<PlaylistItem>()).toStrongRef();
        if (existingParent && existingParent != parent) return -1;
    }
    if (!parent) parent = m_root;
    if (!parent->isList()) return -1;
    if (!parent->isValidIndex(index)) {
        rLog() << "Playlist" << "Invalid index:" << index << "to replace";
        return -1;
    }
    deregisterPlaylist(parent->at(index));
    registerPlaylist(playlist);
    auto currentItem = m_currentItem.toStrongRef();
    bool currentPlaylistReplaced = currentItem && currentItem->parent() == parent->at(index);
    if (currentPlaylistReplaced)
        saveProgress();

    emit aboutToRemove(parent->at(index).data());
    parent->removeAt(index);
    emit removed();
    emit aboutToInsert(parent.data(), index);
    parent->insert(index, playlist);
    emit inserted();

    if (currentPlaylistReplaced)
        setCurrentItem(playlist->getCurrentItem());

    return index;
}

void PlaylistManager::remove(const QModelIndex &index) {
    auto item = static_cast<PlaylistItem*>(index.internalPointer());
    if (!item) return;
    auto parent = item->parent();
    int row = index.row();
    if (!parent || (parent->getCurrentIndex() != -1 && parent->getCurrentItem().data() == item)) return;

    if (item->isList()) {
        deregisterPlaylist(item->sharedFromThis());
    }
    emit aboutToRemove(item);
    parent->removeAt(row);
    emit removed();

    if (parent->isEmpty()) {
        auto grandparent = parent->parent();
        if (grandparent) {
            emit aboutToRemove(parent.data());
            grandparent->removeAt(parent->row());
            emit removed();
        }
    }
    auto currentItem = m_currentItem.toStrongRef();
    setCurrentItem(currentItem);
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
    auto currentItem = m_currentItem.toStrongRef();
    setCurrentItem(currentItem);
}

bool PlaylistManager::playPlaylist(int index) {
    if (m_root->isEmpty()) return false;
    auto playlist = m_root->at(index);
    if (!playlist) return false;
    int itemIndex = (playlist->getCurrentIndex() == -1) ? 0 : playlist->getCurrentIndex();
    return tryPlay(playlist->at(itemIndex));
}

void PlaylistManager::loadNextItem(int offset) {
    auto currentItem = m_currentItem.toStrongRef();
    if (!currentItem) {
        tryPlay(m_root->at(0));
        return;
    }
    auto playlist = currentItem->parent();
    if (!playlist) {
        rLog() << "Playlist" << currentItem->link << "does not belong to a playlist";
        return;
    }

    int nextItemIndex = currentItem->row() + offset;
    auto parentPlaylist = playlist->parent();
    int playlistIndex = playlist->row();
    if (parentPlaylist) {
        if (nextItemIndex == playlist->count() && playlistIndex + 1 < parentPlaylist->count()) {
            loadNextPlaylist(1);
            return;
        } else if (nextItemIndex < 0 && playlistIndex - 1 >= 0) {
            loadNextPlaylist(-1);
            return;
        }
    }
    auto nextItem = playlist->at(nextItemIndex);
    tryPlay(nextItem);
}

void PlaylistManager::loadNextPlaylist(int offset) {
    auto currentItem = m_currentItem.toStrongRef();
    if (!currentItem || !currentItem->parent()) {
        tryPlay(m_root->at(0));
        return;
    }
    auto playlist = currentItem->parent();
    auto parentPlaylist = playlist->parent();
    if (!parentPlaylist) return;
    int row = playlist->row();
    auto nextPlaylist = parentPlaylist->at(row + offset);
    if (!nextPlaylist) return;
    
    auto nextItemIndex = 0;
    if (nextPlaylist->getCurrentIndex() == -1) {
        for (int i = 0; i < nextPlaylist->count(); ++i) {
            if (!nextPlaylist->at(i)->isList()) {
                nextItemIndex = i;
                break;
            }
        }
    } else {
        nextItemIndex = nextPlaylist->getCurrentIndex();
    }

    tryPlay(nextPlaylist->at(nextItemIndex));
}

void PlaylistManager::loadIndex(const QModelIndex &index) {
    auto item = static_cast<PlaylistItem*>(index.internalPointer());
    if (item->isList()) return;
    auto playlist = item->parent();
    if (item) tryPlay(playlist->at(item->row()));
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

    QSharedPointer<PlaylistItem> playlist = nullptr;
    if (url.isLocalFile()) {
        QFileInfo pathInfo(url.toLocalFile());
        QString dirPath = pathInfo.isDir() ? pathInfo.absoluteFilePath() : pathInfo.dir().absolutePath();
        cLog() << "Playlist" << "Opening local file" << dirPath;

        playlist = m_playlistMap.value(dirPath, QWeakPointer<PlaylistItem>()).toStrongRef();
        if (playlist) {
            if (!pathInfo.isDir()) 
                playlist->setCurrentIndex(playlist->indexOf(pathInfo.absoluteFilePath()));
            
        } else {
            playlist = QSharedPointer<PlaylistItem>::create();
            if (loadFromFolder(url, playlist, true)) {
                append(playlist);
                cLog() << "Playlist" << "Loaded folder" << dirPath;
            } else {
                cLog() << "Playlist" << "Failed to load folder" << dirPath;
                playlist = nullptr;
            }
        }
    } else {
        cLog() << "Playlist" << "Opening online video" << urlString;
        playlist = m_playlistMap.value("videos", QWeakPointer<PlaylistItem>()).toStrongRef();
        if (!playlist) {
            playlist = QSharedPointer<PlaylistItem>::create("Videos", nullptr, "videos");
            append(playlist);
        }

        int itemIndex = playlist->indexOf(urlString);
        if (itemIndex == -1) {
            emit aboutToInsert(playlist.data(), playlist->count());
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

void PlaylistManager::reload() {
    auto currentItem = m_currentItem.toStrongRef();
    if (!currentItem) return;
    currentItem->setTimestamp(MpvObject::instance()->time());
    tryPlay(currentItem);
}

void PlaylistManager::loadServer(int index) {
    if (m_watcher.isRunning()) {
        m_cancelled = true;
        return;
    }
    if (!m_serverListModel.isValidIndex(index)) return;
    m_cancelled = false;
    setIsLoading(true);
    m_watcher.setFuture(QtConcurrent::run([&]() {
        Client client(&m_cancelled);
        QString serverName = m_serverListModel.at(index).name;
        PlayInfo playItem = m_serverListModel.loadServer(&client, index);
        if (playItem.videos.isEmpty()) {
            oLog() << "Server" << QString("Failed to load server %1").arg(serverName);
            return playItem;
        }
        playItem.timestamp = MpvObject::instance()->time();
        QMetaObject::invokeMethod(this, [this, index]() {
            m_serverListModel.setCurrentIndex(index);
            m_serverListModel.setPreferredServer(index);
        }, Qt::QueuedConnection);

        return playItem;
    }));
}

void PlaylistManager::showCurrentItemName() const {
    auto currentItem = m_currentItem.toStrongRef();
    if (!currentItem) return;
    auto playlist = currentItem->parent();
    if (!playlist) return;
    QString path = playlist->name;
    auto current = playlist->parent();
    while (current && current != m_root) {
        path = current->name + " | " + path;
        current = current->parent();
    }
    QString displayText = QString("%1\n[%2/%3] %4\n%5")
                              .arg(path,
                                   QString::number(playlist->getCurrentIndex() + 1), QString::number(playlist->count()),
                                   currentItem->displayName.simplified(), QDateTime::currentDateTime().toString("dd/MM/yyyy HH:mm:ss"));

    MpvObject::instance()->showText(displayText);
}

void PlaylistManager::setCurrentItem(QSharedPointer<PlaylistItem> item) {
    if (!item || item->isList()) {
        m_currentItem = QWeakPointer<PlaylistItem>();
        emit updateSelections(nullptr);
        if (item && item->isList())
            oLog() << "Playlist" << "Cannot set current item to a list" << item->link;
        return;
    }
    m_currentItem = item;
    int row = item->row();
    auto parent = item->parent();
    while (parent) {
        parent->setCurrentIndex(row);
        row = parent->row();
        parent = parent->parent();
    }
    emit updateSelections(m_currentItem.toStrongRef().data());
}

void PlaylistManager::saveProgress() const {
    auto currentItem = m_currentItem.toStrongRef();
    if (!currentItem) return;
    auto playlist = currentItem->parent();
    if (!playlist || !playlist->isList()) return;
    int row = currentItem->row();
    int timestamp = MpvObject::instance()->time();
    cLog() << "Playlist" << playlist->name << "Saving | Index =" << row << "| Timestamp =" << timestamp;

    bool isLastItem = row == playlist->count() - 1;
    bool almostFinished = timestamp > (0.95 * MpvObject::instance()->duration());

    if (!isLastItem && almostFinished) timestamp = 0;

    playlist->getCurrentItem()->setTimestamp(timestamp);
    playlist->updateHistoryFile();
    emit progressUpdated(playlist->link, row, timestamp);
}

void PlaylistManager::cancel() {
    if (!m_watcher.isRunning()) return;
    m_cancelled = true;
}

bool PlaylistManager::loadFromFolder(const QUrl &pathUrl, QSharedPointer<PlaylistItem> playlist, bool recursive) {
    QUrl url = !pathUrl.isEmpty() ? pathUrl : QUrl::fromUserInput(playlist->link);
    if (!url.isValid() || !url.isLocalFile()) return false;
    QFileInfo pathInfo(url.toLocalFile());
    if (!pathInfo.exists()) {
        oLog() << "Playlist" << pathInfo.absoluteFilePath() << "doesn't exist";
        return false;
    }
    QDir playlistDir = pathInfo.isDir() ? QDir(url.toLocalFile()) : pathInfo.dir();
    // List files matching playable extensions
    QFileInfoList fileEntries = playlistDir.entryInfoList(m_playableExtensions, QDir::Files | QDir::NoDotAndDotDot);
    // List all directories separately to avoid filtering them out by name filters
    QFileInfoList dirEntries = playlistDir.entryInfoList(QStringList(), QDir::Dirs | QDir::NoDotAndDotDot);

    playlist->name = playlistDir.dirName();
    playlist->displayName = playlistDir.dirName();
    playlist->link = playlistDir.absolutePath();
    playlist->type |= PlaylistItem::Type::LOCAL;
    playlist->clear();

    if (fileEntries.isEmpty() && (!recursive || dirEntries.isEmpty())) return false;

    playlist->historyFile.reset(new QFile(playlistDir.filePath(".mpv.history")));

    QString fileToPlay;
    int timestamp = 0;

    if (playlist->historyFile->exists()) {
        if (fileEntries.isEmpty()) {
            playlist->historyFile->remove();
        } else if (playlist->historyFile->open(QIODevice::ReadOnly | QIODevice::Text)) {
            auto fileData = QTextStream(playlist->historyFile.data()).readAll().trimmed().split(":");
            playlist->historyFile->close();
            fileToPlay = fileData.first();
            if (fileData.size() == 2)
                timestamp = fileData.last().toInt(); // Defaults to 0 if not ok
        } else {
            rLog() << "Playlist" << "Failed to open history file";
        }
    }

    if (fileEntries.contains(pathInfo) && fileToPlay != pathInfo.fileName()) {
        if (playlist->historyFile->open(QIODevice::WriteOnly | QIODevice::Text)) {
            playlist->historyFile->write(pathInfo.fileName().toUtf8());
            playlist->historyFile->close();
            fileToPlay = pathInfo.fileName();
            timestamp = 0;
        } else {
            rLog() << "Playlist" << "Failed to open and update history file";
        }
    }

    static QRegularExpression fileNameRegex{R"((?:[Ss](?<S>\d{1,2})[Ee](?<E>\d{1,3})[\s\-\.]*| (?<episode>\d{2,3}) ?[\s\-]*)(?<title>[^\(\)]+\w)?.*?\.\w{3,4}$)"};
    for (int i = 0; i < fileEntries.count(); ++i) {
        const QFileInfo &fileInfo = fileEntries[i];
        QString path = fileInfo.absoluteFilePath();
        if (fileInfo.isFile()) {
            QRegularExpressionMatch match = fileNameRegex.match(fileInfo.fileName());
            QString title;
            int season = 0;
            float episodeNumber = -1;
            bool ok;
            if (match.hasMatch()) {
                title = match.hasCaptured("title") ? match.captured("title").simplified() : "";
                season = match.hasCaptured("S") ? match.captured("S").simplified().toInt() : 0;
                QString episodeStr = match.hasCaptured("E") ?
                                         match.captured("E").simplified() : (match.hasCaptured("episode") ? match.captured("episode").simplified() : "");
                float ep = episodeStr.toFloat(&ok);
                episodeNumber = ok ? ep : i;
            } else {
                title = fileInfo.baseName().simplified();
                float ep = title.toFloat(&ok);
                if (ok) {
                    episodeNumber = ep;
                    title = "";
                }
            }
            playlist->emplaceBack(season, episodeNumber, path, title, true);

            if (fileInfo.fileName() == fileToPlay) {
                playlist->setCurrentIndex(playlist->count() - 1);
                playlist->last()->setTimestamp(timestamp);
            }

        }
    }
    // Handle directories after files to preserve ordering of episodes first
    if (recursive) {
        Q_FOREACH (const QFileInfo &dirInfo, dirEntries) {
            QString path = dirInfo.absoluteFilePath();
            if (m_playlistMap.contains(path)) continue;
            auto subPlaylist = QSharedPointer<PlaylistItem>::create();
            if (loadFromFolder(QUrl::fromLocalFile(path), subPlaylist) && !subPlaylist->isEmpty()) {
                playlist->append(subPlaylist);
            }
        }
    }
    if (playlist->isEmpty()) return false;
    playlist->sort();
    return true;
}

void PlaylistManager::onLocalDirectoryChanged(const QString &path) {
    auto playlist = m_playlistMap.value(path, QWeakPointer<PlaylistItem>()).toStrongRef();
    if (!playlist) {
        rLog() << "Playlist" << "Untracked path" << path;
        return;
    }
    playlist->updateHistoryFile();
    // Check if the playlist is the one the current item is in
    auto currentItem = m_currentItem.toStrongRef();
    auto currentParent = currentItem ? currentItem->parent() : nullptr;
    bool isCurrentPlaylist = currentParent == playlist;
    QString prevLink = isCurrentPlaylist ? currentItem->link : "";

    cLog() << "Playlist" << "Directory" << path << "has changed";
    deregisterPlaylist(playlist);
    if (loadFromFolder(QUrl::fromLocalFile(path), playlist, true)) {
        registerPlaylist(playlist);
        emit modelReset();
        if (isCurrentPlaylist) {
            auto newCurrentItem = playlist->getCurrentItem();
            setCurrentItem(newCurrentItem);
            emit scrollToCurrentIndex();
            auto currentLink = newCurrentItem ? newCurrentItem->link : "";
            if (currentLink != prevLink) {
                tryPlay(newCurrentItem);
            }
        }
        return;
    }
    cLog() << "Playlist" << "Failed to reload folder" << playlist->link;
    MpvObject::instance()->pause();
    auto parent = playlist->parent();
    emit aboutToRemove(playlist.data());
    parent->removeOne(playlist);
    emit removed();
    setCurrentItem(m_currentItem.toStrongRef());
}

void PlaylistManager::registerPlaylist(QSharedPointer<PlaylistItem> playlist) {
    if (!playlist || !playlist->isList() || m_playlistMap.contains(playlist->link)) return;
    QList<QSharedPointer<PlaylistItem>> items{playlist};
    while (!items.isEmpty()) {
        auto item = items.takeFirst();
        m_playlistMap.insert(item->link, QWeakPointer<PlaylistItem>(item));
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

void PlaylistManager::deregisterPlaylist(QSharedPointer<PlaylistItem> playlist) {
    if (!playlist) return;
    if (!m_playlistMap.contains(playlist->link)) {
        rLog() << "Playlist" << "Attempting to deregister unregistered playlist" << playlist->name;
        return;
    }

    QList<QSharedPointer<PlaylistItem>> items{playlist};
    while (!items.isEmpty()) {
        auto item = items.takeFirst();
        m_playlistMap.remove(item->link);
        if (item->isLocalDir())
            m_folderWatcher.removePath(item->link);

        auto it = item->iterator();
        while (it.hasNext()) {
            auto child = it.next();
            if (!child->isList()) continue;
            items.append(child);
        }
    }
}

bool PlaylistManager::tryPlay(QSharedPointer<PlaylistItem> item) {
    if (!item) return false;
    auto parent = !item->isList() ? item->parent() : nullptr;
    QString link = (!item->isList() ? parent : item)->link;
    auto playlist = !link.isEmpty() ? m_playlistMap.value(link, QWeakPointer<PlaylistItem>()).toStrongRef() : nullptr;
    if (!playlist) {
        rLog() << "Playlist" << (!item->isList() ? item->parent() : item)->link << "is not registered";
        return false;
    }
    if (!item->isList() && parent != playlist) {
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
        m_cancelled = true;
        return false;
    }
    m_cancelled = false;
    setIsLoading(true);
    saveProgress();
    m_serverListModel.clear();
    m_watcher.setFuture(QtConcurrent::run([this, item]() {
        PlayInfo result = this->play(item);
        return result;
    }));
    return true;
}

PlayInfo PlaylistManager::play(QSharedPointer<PlaylistItem> item) {
    QSharedPointer<PlaylistItem> candidate = item;
    while (candidate && candidate->isList()) {
        if (candidate->isEmpty()) return {};
        auto currentItem = candidate->getCurrentItem();
        if (currentItem) {
            candidate = currentItem;
            continue;
        }
        // Prefer first non-list child; if none, dive into first child list
        QSharedPointer<PlaylistItem> firstPlayable = nullptr;
        auto it = candidate->iterator();
        while (it.hasNext()) {
            auto child = it.next();
            if (!child->isList()) { firstPlayable = child; break; }
        }
        candidate = firstPlayable ? firstPlayable : candidate->first();
    }
    if (!candidate) return {};
    item = candidate;
    auto playlist = item->parent();

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
            Q_FOREACH (const QString &headerLine, parts) {
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

        auto servers = provider->loadServers(&m_client, item.data());
        if (servers.isEmpty())
            throw AppException("No servers found for " + item->name, "Server");

        std::sort(servers.begin(), servers.end(),
                  [](const VideoServer &a, const VideoServer &b) {
                      return a.name < b.name;
                  });

        auto result = ServerListModel::findWorkingServer(&m_client, provider, servers);
        if (result.first == -1)
            throw AppException("No working server found for " + item->name, "Server");

        if (m_cancelled) return {};
        int chosenIndex = result.first;
        PlayInfo chosenPlayInfo = result.second;
        QMetaObject::invokeMethod(this, [this, servers, provider, chosenIndex]() {
            m_serverListModel.setServers(servers, provider);
            m_serverListModel.setCurrentIndex(chosenIndex);
            m_serverListModel.setPreferredServer(chosenIndex);
        }, Qt::QueuedConnection);
        playInfo = chosenPlayInfo;
        break;
    }
    case PlaylistItem::LOCAL: {
        if (!QFile::exists(item->link)) {
            oLog() << "Playlist" << item->link << "does not exist";
            bool wasCurrent = (playlist->getCurrentIndex() == itemRow);
            emit aboutToRemove(item.data());
            playlist->removeAt(itemRow);
            emit removed();
            if (wasCurrent)
                playlist->setCurrentIndex(-1);
            return {};
        }
        playInfo.videos.emplaceBack(item->link);
        break;
    }
    case PlaylistItem::LIST: return {};
    }

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

    QMetaObject::invokeMethod(this, [this, plink = playlist->link, irow = itemRow, ts = item->getTimestamp(), chosen = item]() {
        emit progressUpdated(plink, irow, ts);
        setCurrentItem(chosen);
        emit scrollToCurrentIndex();
    }, Qt::QueuedConnection);
    return playInfo;
}
