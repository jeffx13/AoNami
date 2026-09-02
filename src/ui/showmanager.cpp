#include "ui/showmanager.h"
#include "media/playlistitem.h"
#include "providers/showprovider.h"
#include "ui/uibridge.h"
#include "app/logger.h"
#include "app/settings.h"

int ShowManager::getLastWatchedIndex() const {
    auto playlist = m_showObject.getPlaylist();
    return playlist ? playlist->getCurrentIndex() : -1;
}

void ShowManager::setLastWatchedIndex(int index) {
    auto playlist = m_showObject.getPlaylist();
    if (!playlist) return;

    if (playlist->parent() && playlist->parent()->getCurrentIndex() == playlist->row())
        return;

    if (!playlist->setCurrentIndex(index)) return;
    updateContinueEpisode();
    emit lastWatchedIndexChanged();
}

void ShowManager::updateContinueEpisode() {
    auto playlist = m_showObject.getPlaylist();
    if (!playlist) { m_continueText.clear(); return; }

    int idx = qMax(playlist->getCurrentIndex(), 0);

    // Past the threshold the episode is done with; point at the next one.
    if (auto watched = playlist->at(idx);
        watched && watched->getProgress() >= Settings::instance().watchedFraction()
        && idx + 1 < playlist->count())
        ++idx;

    m_continueIndex = idx;
    auto episode = playlist->at(m_continueIndex);
    if (!episode) { m_continueText.clear(); return; }

    m_continueText = (m_continueIndex == 0 ? QStringLiteral("Play ")
                                           : QStringLiteral("Continue from "))
                     + episode->displayName.simplified();
}

void ShowManager::cancel() {
    if (m_watcher.isRunning())
        m_cancel.cancel();
}

void ShowManager::setShow(const ShowData &show, const ShowData::LastWatchInfo &lastWatchInfo, bool navigate) {
    if (m_watcher.isRunning()) {
        m_pendingShow = show;
        m_pendingInfo = lastWatchInfo;
        m_pendingNavigate = navigate;
        m_hasPending = true;
        m_cancel.cancel();
        return;
    }
    if (m_showObject.getShow().link == show.link) {
        if (navigate) UiBridge::instance().navigateTo(UiBridge::Page::Info);
        return;
    }
    m_watcher.setFuture(QtConcurrent::run(&ShowManager::loadShow, this, show, lastWatchInfo, navigate));
}

void ShowManager::reload(const ShowData &show, const ShowData::LastWatchInfo &lastWatchInfo) {
    if (m_watcher.isRunning()) return;
    m_watcher.setFuture(QtConcurrent::run(&ShowManager::loadShow, this, show, lastWatchInfo, false));
}

void ShowManager::onLoadFinished() {
    m_cancel.reset();
    if (!m_hasPending) return;
    m_hasPending = false;
    setShow(m_pendingShow, m_pendingInfo, m_pendingNavigate);
}

void ShowManager::loadShow(const ShowData &show, const ShowData::LastWatchInfo &lastWatchInfo, bool navigate) {
    // Worker thread: local copies only.
    ShowData loadedShow = show;
    auto playlist = lastWatchInfo.playlist;
    const bool usingExistingPlaylist = (playlist != nullptr);

    bool success = false;
    if (loadedShow.provider) {
        cLog() << loadedShow.provider->name() << "Loading" << loadedShow.title << "using" << loadedShow.link;
        try {
            Client client(m_cancel);
            success = loadedShow.provider->loadShow(&client, loadedShow);
        } catch (const std::exception &ex) {
            UiBridge::instance().showError(QString::fromUtf8(ex.what()), loadedShow.provider->name() + " Error");
        } catch (...) {
            UiBridge::instance().showError("An unknown error occurred", loadedShow.provider->name() + " Error");
        }
    }

    if (!success || m_cancel.isCancelled()) {
        if (!success) {
            // A provider returning false (rather than throwing) otherwise looks like a dead click.
            oLog() << "ShowManager" << "Failed to load" << loadedShow.title;
            const QString title = loadedShow.title;
            QMetaObject::invokeMethod(&UiBridge::instance(), [title]() {
                UiBridge::instance().showError("Could not load " + title + ".", "Show Error");
            }, Qt::QueuedConnection);
        }
        return;  // onLoadFinished (watcher) resets the token + clears isLoading
    }

    if (!playlist)
        playlist = loadedShow.getPlaylist();

    bool shouldReverse = false;
    if (usingExistingPlaylist) {
        // Reuse PlaylistManager's playlist - it already tracks watch state.
        shouldReverse = playlist->getCurrentIndex() > 0;
        loadedShow.setPlaylist(playlist);
    } else if (playlist && playlist->isValidIndex(lastWatchInfo.lastWatchedIndex)) {
        playlist->setCurrentIndex(lastWatchInfo.lastWatchedIndex);
        if (auto item = playlist->getCurrentItem())
            item->setProgress(lastWatchInfo.progress);
        shouldReverse = lastWatchInfo.lastWatchedIndex > 0;
    }

    cLog() << "ShowManager" << "Loaded" << loadedShow.title;

    QMetaObject::invokeMethod(this, [this, loadedShow = std::move(loadedShow), playlist, shouldReverse, navigate]() {
        // A newer request arrived - drop this stale result.
        if (m_cancel.isCancelled()) return;
        m_showObject.setShow(loadedShow);
        m_episodeList.setPlaylist(playlist);
        // Unconditional: only setting it kept the previous show's order on an unwatched one.
        m_episodeList.setIsReversed(shouldReverse);
        updateContinueEpisode();
        if (navigate) UiBridge::instance().navigateTo(UiBridge::Page::Info);
        emit showChanged();
        emit lastWatchedIndexChanged();
    }, Qt::QueuedConnection);
}
