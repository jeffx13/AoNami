#include "showmanager.h"
#include "base/player/playlistitem.h"
#include "providers/showprovider.h"

int ShowManager::getLastWatchedIndex() const {
    auto playlist = m_showObject.getPlaylist();
    if (!playlist) return -1;
    return playlist->getCurrentIndex();
}

void ShowManager::setLastWatchedIndex(int index){
    auto playlist = m_showObject.getPlaylist();
    if (!playlist->setCurrentIndex(index))
        return;
    updateContinueEpisode();
    emit lastWatchedIndexChanged();
    // potential bug
    // when playlist is connected to playlistmanager
}

void ShowManager::updateContinueEpisode() {
    auto playlist = m_showObject.getPlaylist();
    if (!playlist) return;

    int currentIndex = playlist->getCurrentIndex();
    m_continueIndex = currentIndex == -1 ? 0 : currentIndex;

    bool isPenultimateItem = currentIndex == playlist->count() - 2;
    m_continueIndex = isPenultimateItem ? playlist->count() - 1 : m_continueIndex;
    PlaylistItem *episode = playlist->at(m_continueIndex);
    m_continueText = currentIndex == -1 ? "Play " : "Continue from ";
    m_continueText += (episode->name.isEmpty() ? QString::number (episode->number) :(episode->number < 0
                                                                                         ? episode->name : QString::number (episode->number) + "\n" + episode->name));
}

void ShowManager::cancel() {
    if (m_watcher.isRunning()){
        m_isCancelled = true;
    }
}

ShowManager::ShowManager(QObject *parent) : ServiceManager(parent) {
    QObject::connect(&m_watcher, &QFutureWatcher<void>::finished, this, [this](){
        if (m_isCancelled.load()) {
            oLog() << "ShowManager" << "Operation cancelled";
        } else {
            emit showChanged();
        }
        m_isCancelled = false;
        setIsLoading(false);
    });
}

void ShowManager::setShow(const ShowData &show, const ShowData::LastWatchInfo &lastWatchInfo) {
    if (m_watcher.isRunning()) {
        m_isCancelled = true;
        return;
    }

    if (m_showObject.getShow().link == show.link) {
        emit showChanged();
        return;
    }

    setIsLoading(true);
    m_watcher.setFuture(QtConcurrent::run(&ShowManager::loadShow, this, show, lastWatchInfo));
}

void ShowManager::loadShow(const ShowData &show, const ShowData::LastWatchInfo &lastWatchInfo) {
    m_showObject.setShow(show);
    m_showObject.getShow().setPlaylist(lastWatchInfo.playlist);
    m_episodeList.setPlaylist(lastWatchInfo.playlist);

    if (m_isCancelled) return;
    bool success = false;
    if (show.provider) {
        cLog() << show.provider->name() << "Loading" << show.title << "using" << show.link;
        try {
            success = show.provider->loadDetails(&m_client, m_showObject.getShow(), false, lastWatchInfo.playlist == nullptr, true);
        } catch(QException& ex) {
            ErrorDisplayer::instance().show (ex.what(), show.provider->name() + " Error");
        }
    }
    if (!success) {
        oLog() << "ShowManager" << "Failed to load" << show.title;
        return;
    }

    if (m_isCancelled) return;
    cLog() << "ShowManager" << "Loaded" << show.title;

    auto playlist = m_showObject.getPlaylist();
    if (playlist){
        if (playlist->isValidIndex(lastWatchInfo.lastWatchedIndex)) {
            playlist->setCurrentIndex(lastWatchInfo.lastWatchedIndex);
            playlist->getCurrentItem()->setTimestamp(lastWatchInfo.timeStamp);
            cLog() << "Playlist" << playlist->name << "| Index:" << lastWatchInfo.lastWatchedIndex << "| Timestamp:" <<  lastWatchInfo.timeStamp;
        }
        m_episodeList.setPlaylist(playlist);
        m_episodeList.setIsReversed(playlist->getCurrentIndex() > 0);
        updateContinueEpisode();
    } else {
        m_continueIndex = -1;
        m_continueText = "";
        return;
    }
    emit lastWatchedIndexChanged();
}
