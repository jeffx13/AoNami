#include "showmanager.h"
#include "utils/errorhandler.h"
#include "Providers/showprovider.h"


ShowManager::ShowManager(QObject *parent) : QObject{parent} {
    connect (&m_watcher, &QFutureWatcher<void>::finished, this, [this](){
        if (!m_watcher.future().isValid()) {
            //future was cancelled
            ErrorHandler::instance().show ("Operation cancelled");
            // m_show = ShowData("", "", "", nullptr);
            // m_episodeList.setPlaylist(nullptr);
            // return;
        }


        m_isLoading = false;
        emit isLoadingChanged();

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
    try {
        show.provider->loadDetails(tempShow);
    } catch(QException& ex) {
        ErrorHandler::instance().show (ex.what(), show.provider->name() + " Error");
        return;
    }


    // Only set the show as the current show if it succeeds loading
    m_show = std::move(tempShow);
    m_show.listType = lastWatchInfo.listType;
    if (m_show.playlist) {
        qInfo() << "Log (ShowManager)： Setting last play info for" << show.title
                 << lastWatchInfo.lastWatchedIndex << lastWatchInfo.timeStamp;
        m_show.playlist->setLastPlayAt(lastWatchInfo.lastWatchedIndex, lastWatchInfo.timeStamp);
    }
    m_episodeList.setPlaylist(m_show.playlist);

    emit showChanged();
    qInfo() << "Log (ShowManager)： Successfully loaded details for" << m_show.title;
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
    m_watcher.setFuture(QtConcurrent::run(&ShowManager::loadShow, this, ShowData(show), lastWatchInfo));
}





void ShowManager::setListType(int listType) {
    m_show.listType = listType;
    emit listTypeChanged();
}
