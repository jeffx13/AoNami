#include "showmanager.h"
#include "utils/errorhandler.h"
#include "Providers/showprovider.h"
#include "utils/logger.h"


ShowManager::ShowManager(QObject *parent) : QObject{parent} {
}

void ShowManager::loadShow(const ShowData &show, const ShowData::LastWatchInfo &lastWatchInfo) {
    if (!show.provider) {
        auto errorMessage = QString("Unable to find a provider for %1").arg(m_show.title);
        throw MyException(errorMessage, "Provider");
        return;
    }
    auto tempShow = ShowData(show);
    cLog() << "ShowManager" << "Loading details for" << show.title
            << "with" << show.provider->name()
            << "using the link:" << show.link;
    bool success = false;
    try {
        success = show.provider->loadDetails(&m_client, tempShow, true, lastWatchInfo.playlist == nullptr, false);
    } catch(QException& ex) {
        if (!m_isCancelled)
            ErrorHandler::instance().show (ex.what(), show.provider->name() + " Error");
    }

    if (success && !m_isCancelled) {
        // Only set the show as the current show if it succeeds loading
        m_show = tempShow;
        emit showChanged();
        m_isCancelled = false;
        setIsLoading(false);

        m_show.setListType(lastWatchInfo.listType);
        if (lastWatchInfo.playlist)
            m_show.setPlaylist(lastWatchInfo.playlist);
        else {
            cLog() << "ShowManager" << "Setting last play info for" << show.title
                    << lastWatchInfo.lastWatchedIndex << lastWatchInfo.timeStamp;
            if (m_show.getPlaylist())
                m_show.getPlaylist()->setLastPlayAt(lastWatchInfo.lastWatchedIndex, lastWatchInfo.timeStamp);
        }
        if (auto playlist = m_show.getPlaylist(); playlist) {
            m_episodeList.setPlaylist(playlist);
            m_episodeList.setIsReversed(playlist->currentIndex > 0);
        } else {
            m_episodeList.setPlaylist(nullptr);
        }
        updateContinueEpisode(false);
        gLog() << "ShowManager" << "Successfully loaded details for" << m_show.title;
    } else {
        oLog() << "ShowManager" << "Operation cancelled or failed";
        m_isCancelled = false;
        setIsLoading(false);
    }
}



void ShowManager::setShow(const ShowData &show, const ShowData::LastWatchInfo &lastWatchInfo) {
    if (m_watcher.isRunning()) return;

    if (m_show.link == show.link) {
        emit showChanged();
        return;
    }

    setIsLoading(true);
    m_watcher.setFuture(QtConcurrent::run(&ShowManager::loadShow, this, show, lastWatchInfo));
}





void ShowManager::setListType(int listType) {
    m_show.setListType(listType);
    emit listTypeChanged();
}

ShowProvider* ShowManager::getProvider() const {
    return m_show.provider;
}
