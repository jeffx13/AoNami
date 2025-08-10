#include "showmanager.h"
#include "utils/errorhandler.h"
#include "Providers/showprovider.h"
#include "utils/logger.h"


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
                ErrorHandler::instance().show (ex.what(), m_show.provider->name() + " Error");
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





// void ShowManager::setListType(int listType) {
//     m_show.setListType(listType);
//     emit listTypeChanged();
// }

ShowProvider* ShowManager::getProvider() const {
    return m_show.provider;
}
