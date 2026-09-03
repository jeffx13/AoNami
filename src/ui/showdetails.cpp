#include "ui/showdetails.h"
#include "media/playlistitem.h"
#include "providers/showprovider.h"
#include "ui/uibridge.h"
#include "app/logger.h"
#include "app/settings.h"

ShowDetails::ShowDetails(QObject *parent) : QObject(parent) {
    connect(&m_watcher, &QFutureWatcher<void>::finished, this, &ShowDetails::onLoadFinished);
    connect(&m_watcher, &QFutureWatcher<void>::started,  this, &ShowDetails::isLoadingChanged);
    connect(&m_watcher, &QFutureWatcher<void>::finished, this, &ShowDetails::isLoadingChanged);
}

ShowDetails::~ShowDetails() {
    m_cancel.cancel();
    waitFor(m_watcher, "ShowDetails load");
}

int ShowDetails::lastWatchedIndex() const {
    auto list = playlist();
    return list ? list->currentIndex() : -1;
}

void ShowDetails::setLastWatchedIndex(int index) {
    auto list = playlist();
    if (!list) return;

    if (list->parent() && list->parent()->currentIndex() == list->row())
        return;

    if (!list->setCurrentIndex(index)) return;
    updateContinueEpisode();
    emit lastWatchedIndexChanged();
}

void ShowDetails::updateContinueEpisode() {
    auto list = playlist();
    if (!list) { m_continueText.clear(); return; }

    int idx = qMax(list->currentIndex(), 0);

    // Past the threshold the episode is done with; point at the next one.
    if (auto watched = list->at(idx);
        watched && watched->progress() >= Settings::instance().watchedFraction()
        && idx + 1 < list->count())
        ++idx;

    m_continueIndex = idx;
    auto episode = list->at(m_continueIndex);
    if (!episode) { m_continueText.clear(); return; }

    m_continueText = (m_continueIndex == 0 ? QStringLiteral("Play ")
                                           : QStringLiteral("Continue from "))
                     + episode->displayName.simplified();
}

void ShowDetails::cancel() {
    if (m_watcher.isRunning())
        m_cancel.cancel();
}

void ShowDetails::setShow(const ShowData &show, const ShowData::LastWatchInfo &lastWatchInfo, bool navigate) {
    if (m_watcher.isRunning()) {
        m_pendingShow = show;
        m_pendingInfo = lastWatchInfo;
        m_pendingNavigate = navigate;
        m_hasPending = true;
        m_cancel.cancel();
        return;
    }
    if (m_show.link == show.link) {
        if (navigate) UiBridge::instance().navigateTo(UiBridge::Page::Info);
        return;
    }
    m_watcher.setFuture(QtConcurrent::run(&ShowDetails::load, this, show, lastWatchInfo, navigate));
}

void ShowDetails::reload(const ShowData &show, const ShowData::LastWatchInfo &lastWatchInfo) {
    if (m_watcher.isRunning()) return;
    m_watcher.setFuture(QtConcurrent::run(&ShowDetails::load, this, show, lastWatchInfo, false));
}

void ShowDetails::onLoadFinished() {
    m_cancel.reset();
    if (!m_hasPending) return;
    m_hasPending = false;
    setShow(m_pendingShow, m_pendingInfo, m_pendingNavigate);
}

// Worker thread; the arguments are by-value copies.
void ShowDetails::load(ShowData show, ShowData::LastWatchInfo lastWatchInfo, bool navigate) {
    auto list = lastWatchInfo.playlist;
    const bool usingExistingPlaylist = (list != nullptr);

    bool success = false;
    if (show.provider) {
        cLog() << show.provider->name() << "Loading" << show.title << "using" << show.link;
        try {
            Client client(m_cancel);
            success = show.provider->loadShow(&client, show);
        } catch (const std::exception &ex) {
            UiBridge::instance().showError(QString::fromUtf8(ex.what()), show.provider->name() + " Error");
        } catch (...) {
            UiBridge::instance().showError("An unknown error occurred", show.provider->name() + " Error");
        }
    }

    if (!success || m_cancel.isCancelled()) {
        if (!success) {
            // A provider returning false (rather than throwing) otherwise looks like a dead click.
            oLog() << "ShowDetails" << "Failed to load" << show.title;
            const QString title = show.title;
            QMetaObject::invokeMethod(&UiBridge::instance(), [title]() {
                UiBridge::instance().showError("Could not load " + title + ".", "Show Error");
            }, Qt::QueuedConnection);
        }
        return;  // onLoadFinished (watcher) resets the token + clears isLoading
    }

    if (!list)
        list = show.playlist();

    bool shouldReverse = false;
    if (usingExistingPlaylist) {
        // Reuse Playlist's item - it already tracks watch state.
        shouldReverse = list->currentIndex() > 0;
        show.setPlaylist(list);
    } else if (list && list->isValidIndex(lastWatchInfo.lastWatchedIndex)) {
        list->setCurrentIndex(lastWatchInfo.lastWatchedIndex);
        if (auto item = list->currentItem())
            item->setProgress(lastWatchInfo.progress);
        shouldReverse = lastWatchInfo.lastWatchedIndex > 0;
    }

    cLog() << "ShowDetails" << "Loaded" << show.title;

    QMetaObject::invokeMethod(this, [this, show = std::move(show), list, shouldReverse, navigate]() {
        // A newer request arrived - drop this stale result.
        if (m_cancel.isCancelled()) return;
        m_show = show;
        m_episodes.setPlaylist(list);
        // Unconditional: only setting it kept the previous show's order on an unwatched one.
        m_episodes.setIsReversed(shouldReverse);
        updateContinueEpisode();
        if (navigate) UiBridge::instance().navigateTo(UiBridge::Page::Info);
        emit showChanged();
        emit lastWatchedIndexChanged();
    }, Qt::QueuedConnection);
}
