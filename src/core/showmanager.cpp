#include "showmanager.h"
#include "utils/errorhandler.h"
#include "Providers/showprovider.h"
#include "utils/logger.h"


ShowManager::ShowManager(QObject *parent) : QObject{parent} {
    QObject::connect(&m_watcher, &QFutureWatcher<void>::finished, this, [this](){
        if (m_isCancelled) {
            cLog() << "ShowManager" << "Operation cancelled";
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
    m_show.setListType(lastWatchInfo.listType);
    m_show.setPlaylist(lastWatchInfo.playlist);
    m_episodeList.setPlaylist(nullptr);
    if (m_isCancelled) return;


    bool success = false;
    if (show.provider) {
        cLog() << "ShowManager" << "Loading details for" << m_show.title
               << "with" << m_show.provider->name()
               << "using the link:" << m_show.link;
        try {
            success = show.provider->loadDetails(&m_client, m_show, false, lastWatchInfo.playlist == nullptr);
        } catch(QException& ex) {
            if (!m_isCancelled)
                ErrorHandler::instance().show (ex.what(), m_show.provider->name() + " Error");
        }
    }

    if (m_isCancelled) return;

    if (success) {
        if (m_show.getPlaylist()){
            cLog() << "ShowManager" << "Setting last play info for" << show.title
                    << lastWatchInfo.lastWatchedIndex << lastWatchInfo.timeStamp;
            m_show.getPlaylist()->setLastPlayAt(lastWatchInfo.lastWatchedIndex, lastWatchInfo.timeStamp);
            m_episodeList.setPlaylist(m_show.getPlaylist());
            m_episodeList.setIsReversed(m_show.getPlaylist()->currentIndex > 0);
        }

        updateContinueEpisode(false);
        gLog() << "ShowManager" << "Successfully loaded details for" << m_show.title;
    } else {
        oLog() << "ShowManager" << "Failed to load details for" << m_show.title;
    }

}



void ShowManager::setShow(const ShowData &show, const ShowData::LastWatchInfo &lastWatchInfo) {
    if (m_watcher.isRunning()) {
        m_isCancelled = true;
        m_watcher.waitForFinished();
    }

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
