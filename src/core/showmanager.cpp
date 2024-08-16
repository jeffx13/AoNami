#include "showmanager.h"
#include "utils/errorhandler.h"
#include "Providers/showprovider.h"


ShowManager::ShowManager(QObject *parent) : QObject{parent} {
    connect (&m_watcher, &QFutureWatcher<void>::finished, this, [this](){
        if (!m_watcher.future().isValid()) {
            //future was cancelled
            // ErrorHandler::instance().show ("Operation cancelled", "Error");
            qDebug() << "Operation cancelled";
            // m_show = ShowData("", "", "", nullptr);
            // m_episodeList.setPlaylist(nullptr);
            // return;
        } else {

        }
        if (!m_isCancelled) {
            emit showChanged();
        }
        m_isCancelled = false;
        setIsLoading(false);
    });
}

void ShowManager::loadShow(const ShowData &show, const ShowData::LastWatchInfo &lastWatchInfo) {
    if (!show.provider) {
        throw MyException(QString("Error: Unable to find a provider for %1").arg(m_show.title));
        return;
    }
    auto tempShow = ShowData(show);
    qInfo() << "Log (ShowManager)： Loading details for" << show.title
            << "with" << show.provider->name()
            << "using the link:" << show.link;
    bool success;


    try {
        success = show.provider->loadDetails(&m_client, tempShow, true, lastWatchInfo.playlist == nullptr);
    } catch(QException& ex) {
        if (!m_isCancelled)
            ErrorHandler::instance().show (ex.what(), show.provider->name() + " Error");
        success = false;
    }

    if (success && !m_isCancelled) {
        // Only set the show as the current show if it succeeds loading
        m_show = std::move(tempShow);
        m_show.setListType (lastWatchInfo.listType);
        if (lastWatchInfo.playlist)
            m_show.setPlaylist(lastWatchInfo.playlist);
        else
            m_show.getPlaylist()->setLastPlayAt(lastWatchInfo.lastWatchedIndex, lastWatchInfo.timeStamp);
        // qDebug() << m_show.getPlaylist()->getDisplayNameAt(0);
        qInfo() << "Log (ShowManager)： Setting last play info for" << show.title
                << lastWatchInfo.lastWatchedIndex << lastWatchInfo.timeStamp;

        m_episodeList.setPlaylist(m_show.getPlaylist());


        qInfo() << "Log (ShowManager)： Successfully loaded details for" << m_show.title;
    }
}


void ShowManager::setShow(const ShowData &show, const ShowData::LastWatchInfo &lastWatchInfo) {
    if (m_watcher.isRunning())
        return;

    if (m_show.link == show.link) {
        emit showChanged();
        return;
    }

    m_isLoading = true;
    emit isLoadingChanged();
    m_watcher.setFuture(QtConcurrent::run(&ShowManager::loadShow, this, show, lastWatchInfo));
}





void ShowManager::setListType(int listType) {
    m_show.setListType(listType);
    emit listTypeChanged();
}
