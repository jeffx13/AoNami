#include "showmanager.h"
#include "utils/errorhandler.h"
#include "Providers/showprovider.h"


ShowManager::ShowManager(QObject *parent) : QObject{parent} {
    connect (&m_watcher, &QFutureWatcher<void>::finished, this, [this](){
        if (!m_watcher.future().isValid()) {
            //future was cancelled
            ErrorHandler::instance().show ("Operation cancelled", "Error");
            // m_show = ShowData("", "", "", nullptr);
            // m_episodeList.setPlaylist(nullptr);
            // return;
        }

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
        success = show.provider->loadDetails(tempShow, lastWatchInfo.playlist == nullptr);
    } catch(QException& ex) {
        ErrorHandler::instance().show (ex.what(), show.provider->name() + " Error");
        return;
    }

    if (success) {
        // Only set the show as the current show if it succeeds loading
        m_show = std::move(tempShow);
        m_show.setListType (lastWatchInfo.listType);
        if (lastWatchInfo.playlist)
            m_show.setPlaylist(lastWatchInfo.playlist);
        else
            m_show.getPlaylist ()->setLastPlayAt(lastWatchInfo.lastWatchedIndex, lastWatchInfo.timeStamp);

        qInfo() << "Log (ShowManager)： Setting last play info for" << show.title
                << lastWatchInfo.lastWatchedIndex << lastWatchInfo.timeStamp;

        m_episodeList.setPlaylist(m_show.getPlaylist ());

        emit showChanged();
        qInfo() << "Log (ShowManager)： Successfully loaded details for" << m_show.title;
    }

    m_isLoading = false;
    emit isLoadingChanged();
    return;
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
