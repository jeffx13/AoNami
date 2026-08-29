#include "media/playlistmanager.h"
#include "app/logger.h"
#include "app/exception.h"
#include "media/mpvplayer.h"
#include "providers/showprovider.h"
#include "ui/uibridge.h"
#include "media/serverselector.h"
#include "net/cloudflare.h"
#include "media/localmedia.h"
#include "media/localmedia.h"
#include "app/settings.h"
#include "app/config.h"
#include <QtConcurrent/QtConcurrentRun>
#include <QDateTime>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <algorithm>
#include <QMetaObject>

PlaylistManager::PlaylistManager(QObject *parent) : TreeModel(parent) {
    registerPlaylist(m_root);
    connect(&m_folderWatcher, &QFileSystemWatcher::directoryChanged, this, &PlaylistManager::onLocalDirectoryChanged);
    connect(&m_watcher, &QFutureWatcher<PlayInfo>::finished, this, &PlaylistManager::onPlayFinished);
    // Ordered after onPlayFinished so isLoading reads the post-handoff state.
    connect(&m_watcher, &QFutureWatcher<PlayInfo>::started,  this, &PlaylistManager::isLoadingChanged);
    connect(&m_watcher, &QFutureWatcher<PlayInfo>::finished, this, &PlaylistManager::isLoadingChanged);

    m_prefetchTimer.setSingleShot(true);
    m_prefetchTimer.setInterval(4000);   // let the current episode buffer before resolving the next
    connect(&m_prefetchTimer, &QTimer::timeout, this, &PlaylistManager::startNextEpisodePrefetch);
}

void PlaylistManager::onPlayFinished() {
    if (!m_cancel.isCancelled()) {
        try {
            auto playItem = m_watcher.result();
            if (auto *mpv = MpvPlayer::instance()) mpv->open(playItem);
        } catch (AppException &ex) {
            ex.show();
        } catch (const std::runtime_error &ex) {
            UiBridge::instance().showError(ex.what(), "Playlist Error");
        } catch (...) {
            UiBridge::instance().showError("Something went wrong", "Playlist Error");
        }
    }
    m_cancel.reset();

    if (m_pendingServerIndex >= 0) {
        int idx = m_pendingServerIndex;
        m_pendingServerIndex = -1;
        loadServer(idx);
    } else if (m_pendingItem) {
        auto item = m_pendingItem;
        m_pendingItem.clear();
        tryPlay(item);
    }
}

PlaylistManager::~PlaylistManager() {
    disconnect(&m_watcher, &QFutureWatcher<PlayInfo>::finished, this, nullptr);
    m_cancel.cancel();
    m_appendCancel.cancel();
    m_bgCacheCancel.cancel();
    m_prefetchCancel.cancel();
    if (m_prefetchFuture.isRunning()) {
        try { m_prefetchFuture.waitForFinished(); } catch (...) { qWarning("PlaylistManager: prefetchFuture threw during shutdown"); }
    }
    if (m_watcher.isRunning()) {
        try { m_watcher.waitForFinished(); } catch (...) { qWarning("PlaylistManager: watcher threw during shutdown"); }
    }
    if (m_appendFuture.isRunning()) {
        try { m_appendFuture.waitForFinished(); } catch (...) { qWarning("PlaylistManager: appendFuture threw during shutdown"); }
    }
    if (m_bgCacheFuture.isRunning()) {
        try { m_bgCacheFuture.waitForFinished(); } catch (...) { qWarning("PlaylistManager: bgCacheFuture threw during shutdown"); }
    }
}

QSharedPointer<PlaylistItem> PlaylistManager::find(const QString &link) {
    auto it = m_playlistMap.find(link);
    return it != m_playlistMap.end() ? it.value().toStrongRef() : nullptr;
}

int PlaylistManager::append(const QSharedPointer<PlaylistItem> &playlist, const QSharedPointer<PlaylistItem> &parent) {
    return insert(INT_MAX, playlist, parent);
}

int PlaylistManager::insert(int index, const QSharedPointer<PlaylistItem> &playlist, const QSharedPointer<PlaylistItem> &parent) {
    if (!playlist) return -1;
    auto actualParent = parent ? parent : m_root;
    if (!actualParent->isList()) return -1;

    auto existingPlaylist = m_playlistMap.value(playlist->link, QWeakPointer<PlaylistItem>()).toStrongRef();
    if (existingPlaylist)
        return existingPlaylist->row();

    registerPlaylist(playlist);
    index = qBound(0, index, actualParent->count());
    beginInsertRows(indexFor(actualParent.data()), index, index);
    actualParent->insert(index, playlist);
    endInsertRows();

    auto currentItem = m_currentItem.toStrongRef();
    setCurrentItem(currentItem);
    return index;
}

bool PlaylistManager::isPlaying(const QString &link) const {
    if (link.isEmpty()) return false;
    for (auto item = m_currentItem.toStrongRef(); item; item = item->parent())
        if (item->link == link) return true;
    return false;
}

void PlaylistManager::rekey(const QString &oldLink, const QSharedPointer<PlaylistItem> &newPlaylist) {
    auto stale = find(oldLink);
    if (!stale) return;
    auto parent = stale->parent();
    if (newPlaylist && replace(stale->row(), newPlaylist, parent == m_root ? nullptr : parent) != -1)
        return;
    remove(indexFor(stale.data()));
}

int PlaylistManager::replace(int index, const QSharedPointer<PlaylistItem> &playlist, const QSharedPointer<PlaylistItem> &parent) {
    if (!playlist) return -1;

    auto existingPlaylist = m_playlistMap.value(playlist->link, QWeakPointer<PlaylistItem>()).toStrongRef();
    if (existingPlaylist)
        return existingPlaylist->row();

    if (parent) {
        auto existingParent = m_playlistMap.value(parent->link, QWeakPointer<PlaylistItem>()).toStrongRef();
        if (existingParent && existingParent != parent) return -1;
    }
    auto actualParent = parent ? parent : m_root;
    if (!actualParent->isList()) return -1;
    if (!actualParent->isValidIndex(index)) {
        rLog() << "Playlist" << "Invalid index:" << index << "to replace";
        return -1;
    }

    deregisterPlaylist(actualParent->at(index));
    registerPlaylist(playlist);

    auto currentItem = m_currentItem.toStrongRef();
    bool currentPlaylistReplaced = currentItem && currentItem->parent() == actualParent->at(index);
    if (currentPlaylistReplaced)
        saveProgress();

    beginRemoveRows(indexFor(actualParent.data()), index, index);
    actualParent->removeAt(index);
    endRemoveRows();
    beginInsertRows(indexFor(actualParent.data()), index, index);
    actualParent->insert(index, playlist);
    endInsertRows();

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

    if (item->isList())
        deregisterPlaylist(item->sharedFromThis());

    beginRemoveRows(indexFor(parent.data()), row, row);
    parent->removeAt(row);
    endRemoveRows();

    if (parent->isEmpty()) {
        auto grandparent = parent->parent();
        if (grandparent) {
            int parentRow = parent->row();
            beginRemoveRows(indexFor(grandparent.data()), parentRow, parentRow);
            grandparent->removeAt(parentRow);
            endRemoveRows();
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
        if (playlist != currentPlaylist)
            deregisterPlaylist(playlist);
    }
    // The reset must bracket the mutation: clear() frees children still inside live QModelIndexes.
    beginResetModel();
    m_root->clear();
    if (currentPlaylist)
        m_root->append(currentPlaylist);
    endResetModel();
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
    if (parentPlaylist) {
        int playlistIndex = playlist->row();
        if (nextItemIndex == playlist->count() && playlistIndex + 1 < parentPlaylist->count()) {
            loadNextPlaylist(1);
            return;
        } else if (nextItemIndex < 0 && playlistIndex - 1 >= 0) {
            loadNextPlaylist(-1);
            return;
        }
    }
    tryPlay(playlist->at(nextItemIndex));
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

    auto nextPlaylist = parentPlaylist->at(playlist->row() + offset);
    if (!nextPlaylist) return;

    int nextItemIndex = 0;
    if (nextPlaylist->getCurrentIndex() != -1) {
        nextItemIndex = nextPlaylist->getCurrentIndex();
    } else {
        for (int i = 0; i < nextPlaylist->count(); ++i) {
            if (!nextPlaylist->at(i)->isList()) {
                nextItemIndex = i;
                break;
            }
        }
    }
    tryPlay(nextPlaylist->at(nextItemIndex));
}

void PlaylistManager::loadIndex(const QModelIndex &index) {
    auto item = static_cast<PlaylistItem*>(index.internalPointer());
    if (!item || item->isList()) return;
    auto playlist = item->parent();
    if (playlist) tryPlay(playlist->at(item->row()));
}

void PlaylistManager::reload() {
    auto currentItem = m_currentItem.toStrongRef();
    if (!currentItem) return;
    if (auto *mpv = MpvPlayer::instance(); mpv && mpv->duration() > 0)
        currentItem->setProgress(double(mpv->time()) / double(mpv->duration()));
    tryPlay(currentItem);
}

void PlaylistManager::loadServer(int index) {
    if (m_watcher.isRunning()) {
        m_pendingServerIndex = index;
        m_cancel.cancel();
        return;
    }
    if (!m_serverListModel.isValidIndex(index)) return;
    VideoServer server = m_serverListModel.at(index);
    ShowProvider *provider = m_serverListModel.provider();
    if (!provider) return;

    cLog() << "Server" << "Switching to" << server.name;

    if (const auto *cached = m_serverListModel.cachedSource(server.name)) {
        PlayInfo playItem = *cached;
        if (auto *mpv = MpvPlayer::instance()) {
            // The cached entry skips checkVideo, so refresh the clearance headers; its cookie may be stale.
            if (!playItem.videos.isEmpty())
                Cloudflare::applyClearanceHeaders(playItem.videos.first().url, playItem.headers);
            if (mpv->duration() > 0)
                playItem.progress = double(mpv->time()) / double(mpv->duration());
            mpv->open(playItem);
        }
        m_serverListModel.setCurrentIndex(index);
        m_serverListModel.setPreferredServer(index);
        cLog() << "Server" << "Loaded" << server.name << "(cached)";
        return;
    }

    m_cancel.reset();
    m_watcher.setFuture(QtConcurrent::run([this, index, server, provider]() {
        Client client(m_cancel);
        PlayInfo playItem = provider->extractSource(&client, server);
        if (!ServerSelector::checkVideo(&client, playItem) && !client.isCancelled()) {
            oLog() << "Server" << server.name << "is broken";
            playItem.clear();
            QMetaObject::invokeMethod(this, [this, name = server.name]() {
                m_serverListModel.markBroken(name);   // keep it visible (greyed), allow retry
            }, Qt::QueuedConnection);
        }
        if (playItem.videos.isEmpty()) {
            oLog() << "Server" << QString("Failed to load server %1").arg(server.name);
            return playItem;
        }
        if (auto *mpv = MpvPlayer::instance(); mpv && mpv->duration() > 0)
            playItem.progress = double(mpv->time()) / double(mpv->duration());
        QMetaObject::invokeMethod(this, [this, serverName = server.name, playItem]() {
            // cacheSource resorts when a broken server recovers, so select by name, not the captured index.
            m_serverListModel.cacheSource(serverName, playItem);
            m_serverListModel.setCurrentServer(serverName);
            cLog() << "Server" << "Loaded" << serverName;
        }, Qt::QueuedConnection);
        return playItem;
    }));
}

void PlaylistManager::tryNextServer() {
    if (m_watcher.isRunning()) return;
    const int current = m_serverListModel.getCurrentIndex();
    if (m_serverListModel.isValidIndex(current))
        m_autoTriedServers.insert(m_serverListModel.at(current).name);

    for (int i = 0; i < m_serverListModel.count(); ++i) {
        if (i == current) continue;
        const QString name = m_serverListModel.at(i).name;
        if (m_autoTriedServers.contains(name)) continue;
        if (!m_serverListModel.cachedSource(name)) continue;   // only known-working servers
        gLog() << "Server" << "Playback failed - auto-switching to" << name;
        loadServer(i);
        return;
    }
    rLog() << "Server" << "Playback failed and no other working server is available";
}

void PlaylistManager::cancel() {
    if (m_watcher.isRunning()) {
        m_cancel.cancel();
    } else if (auto *mpv = MpvPlayer::instance()) {
        if (mpv->isLoading()) mpv->stop();
    }
}

void PlaylistManager::cacheRemainingServers() {
    if (m_bgCacheFuture.isRunning()) return;
    ShowProvider *provider = m_serverListModel.provider();
    if (!provider) return;

    QList<VideoServer> toCheck;
    for (int i = 0; i < m_serverListModel.count(); ++i) {
        const auto &server = m_serverListModel.at(i);
        if (!m_serverListModel.cachedSource(server.name))
            toCheck.append(server);
    }
    if (toCheck.isEmpty()) return;

    m_bgCacheCancel.reset();
    m_bgCacheFuture = QtConcurrent::run([this, toCheck, provider]() {
        // One job per server (<= ~10) - finishes in ~one round-trip instead of serially.
        QList<QFuture<void>> jobs;
        jobs.reserve(toCheck.size());
        for (const VideoServer &server : toCheck) {
            jobs.push_back(QtConcurrent::run([this, server, provider]() {
                if (m_bgCacheCancel.isCancelled()) return;
                bool ok = false;
                PlayInfo playInfo;
                try {
                    Client client(m_bgCacheCancel);
                    playInfo = provider->extractSource(&client, server);
                    ok = !m_bgCacheCancel.isCancelled() && ServerSelector::checkVideo(&client, playInfo);
                } catch (AppException &e) {
                    oLog() << "Server" << server.name << "background cache failed:" << e.what();
                } catch (const std::exception &e) {
                    oLog() << "Server" << server.name << "background cache failed:" << e.what();
                }
                if (m_bgCacheCancel.isCancelled()) return;
                QMetaObject::invokeMethod(this, [this, name = server.name, playInfo, ok]() {
                    if (m_bgCacheCancel.isCancelled()) return;
                    if (ok) m_serverListModel.cacheSource(name, std::move(playInfo));
                    else    m_serverListModel.markBroken(name);
                }, Qt::QueuedConnection);
            }));
        }
        for (auto &j : jobs) j.waitForFinished();
    });
}

void PlaylistManager::appendShow(const QString &title, const QString &link, ShowProvider *provider,
                                 QSharedPointer<PlaylistItem> cached, const ShowData::LastWatchInfo &info, bool play) {
    if (m_appendFuture.isRunning()) return;
    if (!cached) cached = find(link);

    auto commit = [this](const QSharedPointer<PlaylistItem> &pl, const ShowData::LastWatchInfo &inf, bool doPlay) {
        if (inf.lastWatchedIndex != -1) {
            pl->setCurrentIndex(inf.lastWatchedIndex);
            if (auto item = pl->getCurrentItem()) item->setProgress(inf.progress);
        }
        int idx = append(pl);
        if (doPlay) playPlaylist(idx);
    };

    if (cached) { commit(cached, info, play); return; }

    m_appendCancel.reset();
    m_appendFuture = QtConcurrent::run([this, title, link, provider, info, play, commit]() {
        Client client(m_appendCancel);
        ShowData dummy(title, link, "", provider);
        provider->getPlaylist(&client, dummy);
        auto playlist = dummy.getPlaylist();
        if (!m_appendCancel.isCancelled() && playlist) {
            QMetaObject::invokeMethod(this, [this, playlist, info, play, commit]() {
                if (!m_appendCancel.isCancelled()) commit(playlist, info, play);
            }, Qt::QueuedConnection);
        }
    });
}

void PlaylistManager::setCurrentItem(const QSharedPointer<PlaylistItem> &item) {
    if (!item || item->isList()) {
        m_currentItem = QWeakPointer<PlaylistItem>();
        emit currentItemChanged(QModelIndex());
        if (item && item->isList())
            oLog() << "Playlist" << "Cannot set current item to a list" << item->link;
        return;
    }
    m_currentItem = item;
    m_currentCompleted = false;        // new episode - completion re-evaluated from its position
    ensureMpvProgressConnection();

    if (auto *mpv = MpvPlayer::instance()) {
        auto p = item->parent();
        mpv->setShowKey(p ? p->link : item->link);
    }

    int row = item->row();
    auto parent = item->parent();
    while (parent) {
        parent->setCurrentIndex(row);
        row = parent->row();
        parent = parent->parent();
    }
    emit currentItemChanged(indexFor(m_currentItem.toStrongRef().data()));
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
                                   QString::number(playlist->getCurrentIndex() + 1),
                                   QString::number(playlist->count()),
                                   currentItem->displayName.simplified(),
                                   QDateTime::currentDateTime().toString("dd/MM/yyyy HH:mm:ss"));
    if (auto *mpv = MpvPlayer::instance()) mpv->showText(displayText);
}

void PlaylistManager::saveProgress() const {
    auto currentItem = m_currentItem.toStrongRef();
    if (!currentItem || currentItem->preview) return;   // preview/trailer episodes get no resume point
    auto playlist = currentItem->parent();
    if (!playlist || !playlist->isList()) return;

    int row = currentItem->row();
    auto *mpv = MpvPlayer::instance();
    if (!mpv) return;

    const double duration = mpv->duration();
    const double threshold = qBound(1, Settings::instance().watchedPercent(), 100) / 100.0;
    const double progress = duration > 0 ? qBound(0.0, mpv->time() / duration, 1.0) : 0.0;
    const bool completed = duration > 0 && progress >= threshold;
    cLog() << "Playlist" << playlist->name << "Saving | Index =" << row
           << "| Progress =" << QString::number(progress * 100, 'f', 1) + "%";

    auto currentPlaylistItem = playlist->getCurrentItem();
    if (!currentPlaylistItem) return;
    currentPlaylistItem->setProgress(progress);
    playlist->updateHistoryFile();
    emit progressUpdated(playlist->link, row, progress, completed);
}

void PlaylistManager::ensureMpvProgressConnection() {
    if (m_mpvProgressConnected) return;
    auto *mpv = MpvPlayer::instance();
    if (!mpv) return;
    connect(mpv, &MpvPlayer::timeChanged,     this, &PlaylistManager::onPlaybackProgress);
    connect(mpv, &MpvPlayer::durationChanged, this, &PlaylistManager::onPlaybackProgress);
    m_mpvProgressConnected = true;
}

void PlaylistManager::onPlaybackProgress() {
    auto currentItem = m_currentItem.toStrongRef();
    if (!currentItem || currentItem->preview) return;
    auto playlist = currentItem->parent();
    if (!playlist || !playlist->isList()) return;
    auto *mpv = MpvPlayer::instance();
    if (!mpv) return;
    const double duration = mpv->duration();
    if (duration <= 0) return;
    const double threshold = qBound(1, Settings::instance().watchedPercent(), 100) / 100.0;
    const double progress = qBound(0.0, mpv->time() / duration, 1.0);
    const bool completed = progress >= threshold;
    if (completed == m_currentCompleted) return;   // only act on a threshold crossing
    m_currentCompleted = completed;
    currentItem->setProgress(progress);
    emit progressUpdated(playlist->link, currentItem->row(), progress, completed);
}

bool PlaylistManager::tryPlay(const QSharedPointer<PlaylistItem> &item) {
    if (!item) return false;

    auto resolvedItem = item;

    auto parent = resolvedItem->isList() ? nullptr : resolvedItem->parent();
    auto owner = resolvedItem->isList() ? resolvedItem : parent;
    if (!owner) {
        rLog() << "Playlist" << resolvedItem->link << "has no parent playlist";
        return false;
    }
    QString link = owner->link;
    auto playlist = !link.isEmpty() ? m_playlistMap.value(link, QWeakPointer<PlaylistItem>()).toStrongRef() : nullptr;
    if (!playlist) {
        rLog() << "Playlist" << link << "is not registered";
        return false;
    }

    if (!resolvedItem->isList() && parent != playlist) {
        oLog() << "Playlist" << "Item does not belong to registered playlist";
        int itemIndex = playlist->indexOf(resolvedItem->link);
        if (itemIndex != -1) {
            resolvedItem = playlist->at(itemIndex);
        } else {
            rLog() << "Item does not belong to registered playlist";
            return false;
        }
    }

    // Resolve to a playable leaf on the main thread; the worker must not race tree mutations.
    resolvedItem = resolveToPlayableItem(resolvedItem);
    if (!resolvedItem) return false;

    // Keep the leaf's parent alive across the worker (main thread may replace/remove).
    auto playlistRef = resolvedItem->parent();
    if (!playlistRef) return false;

    if (tryUsePrefetch(resolvedItem)) return true;

    if (m_watcher.isRunning()) {
        m_pendingItem = resolvedItem;
        m_cancel.cancel();
        return false;
    }

    // A different episode than any prefetch -> drop the stale one.
    m_prefetchCancel.cancel();
    m_prefetch = {};

    m_pendingItem.clear();
    m_pendingServerIndex = -1;
    m_cancel.reset();

    auto currentItem = m_currentItem.toStrongRef();
    if (currentItem && currentItem != resolvedItem)
        saveProgress();

    m_bgCacheCancel.cancel();
    m_serverListModel.clear();

    m_watcher.setFuture(QtConcurrent::run([this, resolvedItem, playlistRef]() {
        return this->play(resolvedItem);
    }));
    return true;
}

PlayInfo PlaylistManager::play(const QSharedPointer<PlaylistItem> &item) {
    auto playlist = item->parent();
    if (!playlist || !playlist->isList()) {
        rLog() << "Playlist" << item->name << "does not belong to any playlist!";
        return {};
    }

    PlayInfo playInfo = loadPlayInfo(item);
    if (playInfo.videos.isEmpty() && item->type != PlaylistItem::LOCAL)
        return {};

    finalizePlayback(item);
    playInfo.progress = item->getProgress();
    return playInfo;
}

QSharedPointer<PlaylistItem> PlaylistManager::resolveToPlayableItem(QSharedPointer<PlaylistItem> item) {
    while (item && item->isList()) {
        if (item->isEmpty()) return {};

        auto currentItem = item->getCurrentItem();
        if (currentItem) {
            item = currentItem;
            continue;
        }

        QSharedPointer<PlaylistItem> firstPlayable = nullptr;
        auto it = item->iterator();
        while (it.hasNext()) {
            auto child = it.next();
            if (!child->isList()) { firstPlayable = child; break; }
        }
        item = firstPlayable ? firstPlayable : item->first();
    }
    return item;
}

PlayInfo PlaylistManager::loadPlayInfo(const QSharedPointer<PlaylistItem> &item) {
    switch (item->type) {
    case PlaylistItem::PASTED: return loadPastedPlayInfo(item);
    case PlaylistItem::ONLINE: return loadOnlinePlayInfo(item);
    case PlaylistItem::LOCAL:  return loadLocalPlayInfo(item);
    default: return {};
    }
}

PlayInfo PlaylistManager::loadPastedPlayInfo(const QSharedPointer<PlaylistItem> &item) {
    PlayInfo playInfo;
    if (item->link.contains('|')) {
        QStringList parts = item->link.split('|');
        playInfo.videos.emplaceBack(parts.takeFirst());
        for (const QString &headerLine : std::as_const(parts)) {
            QStringList keyValue = headerLine.split(": ", Qt::KeepEmptyParts);
            if (keyValue.size() == 2)
                playInfo.headers.insert(keyValue[0].trimmed(), keyValue[1].trimmed());
        }
    } else {
        playInfo.videos.emplaceBack(item->link);
    }
    return playInfo;
}

PlayInfo PlaylistManager::loadOnlinePlayInfo(const QSharedPointer<PlaylistItem> &item) {
    auto playlist = item->parent();
    if (!playlist)
        throw AppException("Playlist was removed during playback", "Playlist");
    auto provider = playlist->getProvider();
    if (!provider)
        throw AppException("Cannot get provider from playlist!", "Provider");

    QString label = item->displayName;
    label.replace('\n', " - ");
    if (!playlist->name.isEmpty()) label = playlist->name + " " + label;
    label += " (" + provider->name() + ")";

    Client client(m_cancel);
    auto servers = provider->loadServers(&client, item.data());
    if (servers.isEmpty())
        throw AppException("No servers found for " + label, "Server");

    std::sort(servers.begin(), servers.end(),
              [](const VideoServer &a, const VideoServer &b) {
                  return a.name < b.name;
              });

    auto result = ServerSelector::findWorkingServer(&client, provider, servers);
    if (!result.found())
        throw AppException(QString("No working server found for %1 (tried %2)")
                               .arg(label).arg(servers.size()), "Server");

    if (m_cancel.isCancelled()) return {};

    int chosenIndex = result.index;
    QMetaObject::invokeMethod(this, [this, servers, provider, chosenIndex,
                                     cache = std::move(result.cachedSources)]() mutable {
        applyServerResult(servers, provider, chosenIndex, std::move(cache));
    }, Qt::QueuedConnection);

    return result.playInfo;
}

void PlaylistManager::applyServerResult(const QList<VideoServer> &servers, ShowProvider *provider,
                                        int chosenIndex, QHash<QString, PlayInfo> cache) {
    m_autoTriedServers.clear();   // fresh episode - every server gets a chance again
    const QString winnerName = (chosenIndex >= 0 && chosenIndex < servers.size())
                                   ? servers[chosenIndex].name : QString();
    m_serverListModel.setServers(servers, provider);   // sorts internally
    m_serverListModel.setCachedSources(std::move(cache));
    m_serverListModel.setCurrentServer(winnerName);    // locate the winner by name post-sort
    cacheRemainingServers();
}

// Next playable leaf in the same playlist; cross-playlist boundaries aren't prefetched.
QSharedPointer<PlaylistItem> PlaylistManager::computeNextItem() const {
    auto cur = m_currentItem.toStrongRef();
    if (!cur) return {};
    auto pl = cur->parent();
    if (!pl) return {};
    int ni = cur->row() + 1;
    if (ni < 0 || ni >= pl->count()) return {};
    return pl->at(ni);
}

void PlaylistManager::prefetchNextEpisode() {
    auto next = computeNextItem();
    if (!next || next->isList() || next->type != PlaylistItem::ONLINE) {
        m_prefetchTimer.stop();
        return;
    }
    if (m_prefetch.valid && m_prefetch.itemLink == next->link) return;
    m_prefetchCancel.cancel();
    m_prefetch = {};
    m_prefetchTimer.start();
}

void PlaylistManager::startNextEpisodePrefetch() {
    if (m_watcher.isRunning()) { m_prefetchTimer.start(); return; }   // busy resolving - retry later
    auto next = computeNextItem();
    if (!next || next->isList() || next->type != PlaylistItem::ONLINE) return;
    if (m_prefetch.valid && m_prefetch.itemLink == next->link) return;
    auto pl = next->parent();
    ShowProvider *provider = pl ? pl->getProvider() : nullptr;
    if (!provider) return;

    m_prefetchCancel.reset();
    const QString link = next->link;
    m_prefetchFuture = QtConcurrent::run([this, next, provider, link]() {
        if (m_prefetchCancel.isCancelled()) return;
        try {
            Client client(m_prefetchCancel);
            auto servers = provider->loadServers(&client, next.data());
            if (servers.isEmpty() || m_prefetchCancel.isCancelled()) return;
            std::sort(servers.begin(), servers.end(),
                      [](const VideoServer &a, const VideoServer &b) { return a.name < b.name; });
            auto result = ServerSelector::findWorkingServer(&client, provider, servers);
            if (!result.found() || m_prefetchCancel.isCancelled()) return;
            QMetaObject::invokeMethod(this, [this, link, servers, provider,
                                             idx = result.index,
                                             cache = std::move(result.cachedSources),
                                             info = result.playInfo]() mutable {
                if (m_prefetchCancel.isCancelled()) return;
                m_prefetch = Prefetch{ true, link, servers, provider, idx, std::move(cache), info };
                gLog() << "Playlist" << "Prefetched next episode source:" << link;
            }, Qt::QueuedConnection);
        } catch (AppException &e) {
            oLog() << "Playlist" << "Next-episode prefetch failed:" << e.what();
        } catch (const std::exception &e) {
            oLog() << "Playlist" << "Next-episode prefetch failed:" << e.what();
        }
    });
}

bool PlaylistManager::tryUsePrefetch(const QSharedPointer<PlaylistItem> &item) {
    if (!m_prefetch.valid || m_prefetch.itemLink != item->link) return false;
    if (item->type != PlaylistItem::ONLINE) return false;
    if (m_watcher.isRunning()) return false;   // a resolve is mid-flight -> take the normal path

    Prefetch pf = std::move(m_prefetch);
    m_prefetch = {};
    m_prefetchCancel.cancel();
    m_prefetchTimer.stop();

    auto currentItem = m_currentItem.toStrongRef();
    if (currentItem && currentItem != item) saveProgress();

    m_pendingItem.clear();
    m_pendingServerIndex = -1;
    m_bgCacheCancel.cancel();
    m_serverListModel.clear();

    applyServerResult(pf.servers, pf.provider, pf.chosenIndex, std::move(pf.cachedSources));
    finalizePlayback(item);

    PlayInfo playInfo = pf.playInfo;
    playInfo.progress = item->getProgress();
    gLog() << "Playlist" << "Using prefetched source for" << item->displayName;
    if (auto *mpv = MpvPlayer::instance()) mpv->open(playInfo);
    return true;
}

PlayInfo PlaylistManager::loadLocalPlayInfo(const QSharedPointer<PlaylistItem> &item) {
    if (!QFile::exists(item->link)) {
        oLog() << "Playlist" << item->link << "does not exist";
        QMetaObject::invokeMethod(this, [this, item]() {
            auto playlist = item->parent();
            if (!playlist) return;
            int itemRow = item->row();
            if (itemRow < 0) return;
            bool wasCurrent = playlist->getCurrentIndex() == itemRow;
            beginRemoveRows(indexFor(playlist.data()), itemRow, itemRow);
            playlist->removeAt(itemRow);
            endRemoveRows();
            if (wasCurrent) playlist->setCurrentIndex(-1);
        }, Qt::QueuedConnection);
        return {};
    }
    PlayInfo playInfo;
    playInfo.videos.emplaceBack(item->link);
    return playInfo;
}

void PlaylistManager::finalizePlayback(const QSharedPointer<PlaylistItem> &item) {
    QMetaObject::invokeMethod(this, [this, item]() {
        auto playlist = item->parent();
        if (!playlist) return;  // item may have been removed on main thread

        int itemRow = item->row();
        if (playlist->getCurrentIndex() != itemRow) {
            playlist->setCurrentIndex(itemRow);
            playlist->updateHistoryFile();
        }
        int row = playlist->row();
        auto parent = playlist->parent();
        while (parent) {
            parent->setCurrentIndex(row);
            row = parent->row();
            parent = parent->parent();
        }
        if (!item->preview) {
            // Duration is not known yet, so carry the stored fraction rather than resetting it to 0.
            emit progressUpdated(playlist->link, itemRow, item->getProgress(), false);
            emit episodeStarted(playlist->link, itemRow);
        }
        setCurrentItem(item);
        prefetchNextEpisode();
    }, Qt::QueuedConnection);
}

void PlaylistManager::openUrl(QUrl url, bool play) {
    LocalMedia::ParsedUrl parsed = LocalMedia::parse(url);

    if (!parsed.valid) {
        rLog() << "Playlist" << "Invalid url:" << parsed.raw;
        return;
    }
    static QStringList subtitleExtensions = { "srt", "sub", "ssa", "ass", "idx", "vtt" };
    if (subtitleExtensions.contains(QFileInfo(parsed.url.path()).suffix()) ||
        parsed.url.path().toLower().contains("subtitle")) {
        if (auto *mpv = MpvPlayer::instance()) mpv->addSubtitle(Track(parsed.url));
        return;
    }

    if (parsed.url.isLocalFile()) {
        openLocalPath(parsed.url, parsed.raw, play);
    } else {
        openRemoteUrl(parsed.raw, parsed.url, play);
    }
}

void PlaylistManager::openLocalPath(const QUrl &url, const QString &urlString, bool play) {
    QFileInfo pathInfo(url.toLocalFile());
    QString dirPath = pathInfo.isDir() ? pathInfo.absoluteFilePath() : pathInfo.dir().absolutePath();
    cLog() << "Playlist" << "Opening local file" << dirPath;

    auto playlist = m_playlistMap.value(dirPath, QWeakPointer<PlaylistItem>()).toStrongRef();
    if (playlist) {
        if (!pathInfo.isDir())
            playlist->setCurrentIndex(playlist->indexOf(pathInfo.absoluteFilePath()));
    } else {
        playlist = QSharedPointer<PlaylistItem>::create();
        if (LocalMedia::loadFolder(url, playlist, [this](const QString &p) { return m_playlistMap.contains(p); }, 0, 5)) {
            append(playlist);
            cLog() << "Playlist" << "Loaded folder" << dirPath;
        } else {
            cLog() << "Playlist" << "Failed to load folder" << dirPath;
            playlist = nullptr;
        }
    }

    if (playlist && play) {
        if (auto *mpv = MpvPlayer::instance()) mpv->showText(QString("Playing: %1").arg(urlString));
        tryPlay(playlist);
    }
}

void PlaylistManager::openRemoteUrl(const QString &urlString, const QUrl &url, bool play) {
    cLog() << "Playlist" << "Opening online video" << urlString;

    auto playlist = m_playlistMap.value("videos", QWeakPointer<PlaylistItem>()).toStrongRef();
    if (!playlist) {
        playlist = QSharedPointer<PlaylistItem>::create("Videos", nullptr, "videos");
        append(playlist);
    }

    int itemIndex = playlist->indexOf(urlString);
    if (itemIndex == -1) {
        beginInsertRows(indexFor(playlist.data()), playlist->count(), playlist->count());
        playlist->emplaceBack(0, playlist->count() + 1, urlString, url.toString(), false);
        endInsertRows();
        playlist->last()->type = PlaylistItem::PASTED;
        itemIndex = playlist->count() - 1;
    }
    playlist->setCurrentIndex(itemIndex);

    if (play) {
        if (auto *mpv = MpvPlayer::instance()) mpv->showText(QString("Playing: %1").arg(urlString));
        tryPlay(playlist);
    }
}

void PlaylistManager::onLocalDirectoryChanged(const QString &path) {
    auto playlist = m_playlistMap.value(path, QWeakPointer<PlaylistItem>()).toStrongRef();
    if (!playlist) {
        rLog() << "Playlist" << "Untracked path" << path;
        return;
    }
    playlist->updateHistoryFile();

    auto currentItem = m_currentItem.toStrongRef();
    auto currentParent = currentItem ? currentItem->parent() : nullptr;
    bool isCurrentPlaylist = currentParent == playlist;
    QString prevLink = isCurrentPlaylist ? currentItem->link : "";

    cLog() << "Playlist" << "Directory" << path << "has changed";
    deregisterPlaylist(playlist);
    // load() rebuilds the children, so it has to happen inside the reset bracket.
    beginResetModel();
    const bool loaded = LocalMedia::loadFolder(QUrl::fromLocalFile(path), playlist,
                                                [this](const QString &p) { return m_playlistMap.contains(p); });
    endResetModel();
    if (loaded) {
        registerPlaylist(playlist);
        if (isCurrentPlaylist) {
            auto newCurrentItem = playlist->getCurrentItem();
            setCurrentItem(newCurrentItem);
            auto currentLink = newCurrentItem ? newCurrentItem->link : "";
            if (currentLink != prevLink)
                tryPlay(newCurrentItem);
        }
        return;
    }

    cLog() << "Playlist" << "Failed to reload folder" << playlist->link;
    if (auto *mpv = MpvPlayer::instance()) mpv->pause();
    auto parent = playlist->parent();
    if (!parent) return;
    int plRow = playlist->row();
    beginRemoveRows(indexFor(parent.data()), plRow, plRow);
    parent->removeOne(playlist);
    endRemoveRows();
    setCurrentItem(m_currentItem.toStrongRef());
}

void PlaylistManager::visitListNodes(const QSharedPointer<PlaylistItem> &root, const PlaylistVisitor &visitor) {
    QList<QSharedPointer<PlaylistItem>> queue{root};
    for (int front = 0; front < queue.size(); ++front) {
        const auto &item = queue[front];
        visitor(item);
        auto it = item->iterator();
        while (it.hasNext()) {
            auto child = it.next();
            if (child->isList())
                queue.append(child);
        }
    }
}

void PlaylistManager::registerPlaylist(const QSharedPointer<PlaylistItem> &playlist) {
    if (!playlist || !playlist->isList() || m_playlistMap.contains(playlist->link)) return;
    visitListNodes(playlist, [this](const QSharedPointer<PlaylistItem> &item) {
        m_playlistMap.insert(item->link, QWeakPointer<PlaylistItem>(item));
        if (item->isLocalDir())
            m_folderWatcher.addPath(item->link);
    });
}

void PlaylistManager::deregisterPlaylist(const QSharedPointer<PlaylistItem> &playlist) {
    if (!playlist || !m_playlistMap.contains(playlist->link)) return;
    visitListNodes(playlist, [this](const QSharedPointer<PlaylistItem> &item) {
        m_playlistMap.remove(item->link);
        if (item->isLocalDir())
            m_folderWatcher.removePath(item->link);
    });
}

QModelIndex PlaylistManager::getCurrentIndex(const QModelIndex &idx) const {
    auto currentPlaylist = static_cast<PlaylistItem*>(idx.internalPointer());
    if (!currentPlaylist) return QModelIndex();
    int index = currentPlaylist->getCurrentIndex();
    if (!currentPlaylist->isValidIndex(index)) index = 0;
    if (!currentPlaylist->isValidIndex(index)) return QModelIndex();
    return createIndex(index, 0, currentPlaylist->at(index).data());
}

QString PlaylistManager::currentShowName() const {
    auto cur = m_currentItem.toStrongRef();
    auto parent = cur ? cur->parent() : nullptr;
    return parent ? parent->name : QString();
}

QString PlaylistManager::currentItemName() const {
    auto cur = m_currentItem.toStrongRef();
    return cur ? cur->displayName : QString();
}

int PlaylistManager::currentShowEpisodeCount() const {
    auto cur = m_currentItem.toStrongRef();
    auto parent = cur ? cur->parent() : nullptr;
    return parent ? parent->count() : 0;
}

int PlaylistManager::rowCount(const QModelIndex &parent) const {
    if (parent.column() > 0) return 0;
    PlaylistItem *parentItem = parent.isValid()
                                   ? static_cast<PlaylistItem*>(parent.internalPointer())
                                   : m_root.data();
    return parentItem->count();
}

QModelIndex PlaylistManager::index(int row, int column, const QModelIndex &parent) const {
    if (!hasIndex(row, column, parent)) return QModelIndex();
    PlaylistItem *parentItem = parent.isValid()
                                   ? static_cast<PlaylistItem*>(parent.internalPointer())
                                   : m_root.data();
    auto childItem = parentItem->at(row).data();
    return childItem ? createIndex(row, column, childItem) : QModelIndex();
}

QModelIndex PlaylistManager::parent(const QModelIndex &childIndex) const {
    if (!childIndex.isValid()) return QModelIndex();
    auto *childItem = static_cast<PlaylistItem*>(childIndex.internalPointer());
    auto parentItem = childItem ? childItem->parent() : nullptr;
    if (!parentItem || parentItem.get() == m_root.data())
        return QModelIndex();
    return createIndex(parentItem->row(), 0, parentItem.data());
}

QVariant PlaylistManager::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) return QVariant();
    auto *item = static_cast<PlaylistItem*>(index.internalPointer());
    if (!item) return QVariant();

    switch (role) {
    case Qt::DisplayRole:
        return item->isList() ? item->name : item->displayName;
    case TitleRole:
        return item->name;
    case IndexRole:
        return index;
    case NumberRole:
        return item->number;
    case IsCurrentIndexRole: {
        auto parent = item->parent();
        if (!parent || parent->getCurrentIndex() == -1) return false;
        return parent->getCurrentIndex() == item->row();
    }
    case IsDeletableRole:
        return (item->count() > 0) || ((item->type & PlaylistItem::PASTED) != 0);
    case LinkRole:
        return item->link;
    case IsWatchedRole: {
        if (item->isList()) return false;
        auto parent = item->parent();
        return parent && parent->getCurrentIndex() > item->row();
    }
    default:
        return QVariant();
    }
}

bool PlaylistManager::isFilteredOut(const QModelIndex &index, const QString &filter) const {
    if (filter.isEmpty() || !index.isValid()) return false;
    auto *item = static_cast<PlaylistItem*>(index.internalPointer());
    if (!item || item->isList()) return false;
    const QString f = filter.toLower();
    if (item->displayName.toLower().contains(f)) return false;
    if (QString::number(item->number).contains(filter)) return false;
    return true;
}

QHash<int, QByteArray> PlaylistManager::roleNames() const {
    return {
        {TitleRole, "title"},
        {NumberRole, "number"},
        {IndexRole, "index"},
        {Qt::DisplayRole, "display"},
        {IsCurrentIndexRole, "isCurrentIndex"},
        {IsDeletableRole, "isDeletable"},
        {LinkRole, "link"},
        {IsWatchedRole, "isWatched"},
    };
}