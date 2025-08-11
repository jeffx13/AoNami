#include "showmanager.h"
#include "gui/errordisplayer.h"
#include "providers/showprovider.h"
#include "app/logger.h"


ShowManager::ShowManager(QObject *parent) : QObject{parent} {
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

void ShowManager::loadShow(const ShowData &show, const ShowData::LastWatchInfo &lastWatchInfo) {
    // if (!show.provider) {
    //     auto errorMessage = QString("Unable to find a provider for %1").arg(show.title);
    //     throw MyException(errorMessage, "Provider");
    // }
    m_show = show;
    // m_show.setListType(lastWatchInfo.listType);
    m_show.setPlaylist(lastWatchInfo.playlist);
    m_episodeList.setPlaylist(nullptr);

    if (m_isCancelled.load()) return;

    bool success = false;
    if (show.provider) {
        cLog() << m_show.provider->name() << "Loading" << m_show.title << "using" << m_show.link;
        try {
            success = show.provider->loadDetails(&m_client, m_show, false, lastWatchInfo.playlist == nullptr, true);
        } catch(QException& ex) {
            if (!m_isCancelled.load())
                ErrorDisplayer::instance().show (ex.what(), m_show.provider->name() + " Error");
        }
    }

    if (m_isCancelled.load()) return;

    if (success) {
        auto playlist = m_show.getPlaylist();
        cLog() << "ShowManager" << "Loaded" << show.title;
        if (playlist){
            playlist->setLastPlayAt(lastWatchInfo.lastWatchedIndex, lastWatchInfo.timeStamp);
            m_episodeList.setPlaylist(playlist);
            m_episodeList.setIsReversed(playlist->getCurrentIndex() > 0);
        }
        updateContinueEpisode(false);

    } else {
        oLog() << show.provider->name() << "Failed to load" << m_show.title;
    }

}



void ShowManager::setShow(const ShowData &show, const ShowData::LastWatchInfo &lastWatchInfo) {
    if (m_watcher.isRunning()) {
        m_isCancelled = true;
        return;
    }

    if (m_show.link == show.link) {
        emit showChanged();
        return;
    }

    setIsLoading(true);
    m_watcher.setFuture(QtConcurrent::run(&ShowManager::loadShow, this, show, lastWatchInfo));
}

void ShowManager::setLastWatchedIndex(int index) {
    auto playlist = m_show.getPlaylist();
    if (!playlist || playlist->getCurrentIndex() == index || !playlist->isValidIndex(index))
        return;
    playlist->setCurrentIndex(index);
    updateContinueEpisode(true);
}

void ShowManager::updateContinueEpisode(bool notify) {
    if (auto playlist = m_show.getPlaylist(); playlist){
        // If the index in second to last of the latest episode then continue from latest episode
        m_continueIndex = playlist->getCurrentIndex() < 0 ? 0 : playlist->getCurrentIndex();
        const PlaylistItem *episode = playlist->at(m_continueIndex);
        m_continueText = playlist->getCurrentIndex() == -1 ? "Play " : "Continue from ";
        m_continueText += episode->name.isEmpty()
                              ? QString::number (episode->number)
                              : episode->number < 0
                                    ? episode->name
                                    : QString::number (episode->number) + "\n" + episode->name;

    } else {
        m_continueIndex = -1;
        m_continueText = "";
    }
    if (notify)
        emit lastWatchedIndexChanged();

}

int ShowManager::getLastWatchedIndex() const {
    auto currentPlaylist = m_show.getPlaylist();
    if (!currentPlaylist || currentPlaylist->getCurrentIndex() == -1) return -1;
    return currentPlaylist->getCurrentIndex();;
}





// void ShowManager::setListType(int listType) {
//     m_show.setListType(listType);
//     emit listTypeChanged();
// }

ShowProvider* ShowManager::getProvider() const {
    return m_show.provider;
}
