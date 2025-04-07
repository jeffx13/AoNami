#include "application.h"
#include <QNetworkProxyFactory>
#include <QTextCodec>
#include <libxml2/libxml/parser.h>
#include <QQmlContext>
#include <QFontDatabase>
#include <QQuickStyle>

#include "providers/dm84.h"
#include "utils/errorhandler.h"
#include "player/mpvObject.h"
#include "utils/logger.h"
#include "providers/iyf.h"

#include "providers/haitu.h"
#include "providers/allanime.h"
#include "providers/tangrenjie.h"
#include "providers/wco.h"
#include "providers/autoembed.h"
#include "providers/bilibili.h"



Application::Application(QGuiApplication &app, const QString &launchPath,
                         QObject *parent) : QObject(parent), app(app){
    xmlInitParser();
    // curl_global_init(CURL_GLOBAL_ALL);
    QNetworkProxyFactory::setUseSystemConfiguration(true);

    // qputenv("HTTP_PROXY", QByteArray("http://127.0.0.1:7897"));
    // qputenv("HTTPS_PROXY", QByteArray("http://127.0.0.1:7897"));

    REGISTER_QML_SINGLETON(Application, this);
    REGISTER_QML_SINGLETON(ErrorHandler, &ErrorHandler::instance());
    Config::load();
    m_providerManager.setProviders(QList<ShowProvider*>{
        new AllAnime(this),
        new Bilibili(this),
        new IyfProvider(this),

        new Dm84(this),
        new Haitu(this),
        new Tangrenjie(this),
        new Autoembed(this),
        new WCOFun(this),

    });

    if (!launchPath.isEmpty()) {
        QUrl url = QUrl::fromUserInput(launchPath);
        m_playlistManager.openUrl(url, false);
    }




    QObject::connect(&m_showManager, &ShowManager::showChanged,
                     this, [&](){
                    m_libraryManager.updateShowCover(m_showManager.getShow());
                });

    QObject::connect(&m_playlistManager, &PlaylistManager::currentIndexChanged,
                     this, &Application::updateLastWatchedIndex);

    QObject::connect(&m_showManager, &ShowManager::lastWatchedIndexChanged,
                     this, [&](){
                         auto playlist = m_showManager.getPlaylist();
                         m_libraryManager.updateLastWatchedIndex(playlist->link, playlist->currentIndex, 0);
                     });

    QObject::connect(&m_playlistManager, &PlaylistManager::currentIndexAboutToChange,
                     this, &Application::updateTimeStamp);



    // QString tempDir = QDir::tempPath() + "/kyokou";
    // QDir dir(tempDir);
    // if (!dir.exists()) {
    //     if (!dir.mkpath(".")) {
    //         qWarning() << "Failed to create directory:" << tempDir;
    //     }
    // }

    const QUrl url(QStringLiteral("qrc:src/qml/main.qml"));
    setFont(":/resources/app-font.ttf");
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated, &app,
        [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);
    std::setlocale(LC_NUMERIC, "C");
    qputenv("LC_NUMERIC", QByteArrayLiteral("C"));
    QQuickStyle::setStyle("Universal");

    engine.load(url);
}

Application::~Application() {
    // curl_global_cleanup();
    xmlCleanupParser();
    // auto tempFolder = QDir::tempPath() + "/kyokou";
    // if (QDir(tempFolder).exists()) {
    //     QDir(tempFolder).removeRecursively();
    // }
}

void Application::setFont(QString fontPath) {
    qint32 fontId = QFontDatabase::addApplicationFont(fontPath);
    QStringList fontList = QFontDatabase::applicationFontFamilies(fontId);
    QString family = fontList.first();
    QGuiApplication::setFont(QFont(family, 16));
}

void Application::loadShow(int index, bool fromWatchList) {
    if (fromWatchList) {
        QJsonObject showJson = m_libraryManager.getShowJsonAt(index);
        if (showJson.isEmpty()) return;

        QString providerName = showJson["provider"].toString();
        ShowProvider *provider = m_providerManager.getProvider(providerName);
        ShowData show = ShowData::fromJson(showJson, provider);
        int lastWatchedIndex = showJson["lastWatchedIndex"].toInt();
        int timeStamp = showJson["timeStamp"].toInt(0);
        ShowData::LastWatchInfo lastWatchedInfo{ m_libraryManager.getCurrentListType(), lastWatchedIndex, timeStamp };
        lastWatchedInfo.playlist = m_playlistManager.findPlaylist(show.link);
        m_showManager.setShow(show, lastWatchedInfo);
        if (!provider)
            ErrorHandler::instance().show(providerName + " does not exist", "Show Error");
    } else {
        ShowData show = m_searchResultManager.at(index);
        ShowData::LastWatchInfo lastWatchedInfo = m_libraryManager.getLastWatchInfo(show.link);
        lastWatchedInfo.playlist = m_playlistManager.findPlaylist(show.link);
        m_showManager.setShow(show, lastWatchedInfo);
    }
}

void Application::addCurrentShowToLibrary(int listType) {
    m_libraryManager.add(m_showManager.getShow(), listType); // Either changes the list type or adds to library
    m_showManager.setListType(listType);
}

void Application::removeCurrentShowFromLibrary() {
    m_libraryManager.remove(m_showManager.getShow());
    m_showManager.setListType(-1);
}

void Application::downloadCurrentShow(int startIndex, int endIndex) {
    m_downloadManager.downloadShow (m_showManager.getShow(), startIndex, endIndex);
}

void Application::playFromEpisodeList(int index, bool append) {
    auto showPlaylist = m_showManager.getPlaylist();
    if (!showPlaylist) return;

    if (m_playlistManager.isLoading()) {
        m_playlistManager.cancel();
        return;
    }

    bool inPlaylistManager = m_playlistManager.findPlaylist(showPlaylist->link);
    if (!inPlaylistManager) {
        updateTimeStamp();
    }

    if (append) {
        m_playlistManager.append(showPlaylist);
        showPlaylist->setLastPlayAt(index, 0);
    } else {
        showPlaylist->seasonNumber = -1; // mark this as an online playlist which is always the first playlist
        auto firstPlaylist = m_playlistManager.at(0);

        int playlistIndex = 0;
        if (firstPlaylist && firstPlaylist->seasonNumber == -1) {
            playlistIndex = m_playlistManager.replace(0, showPlaylist);
        } else {
            playlistIndex = m_playlistManager.insert(0, showPlaylist);
        }

        m_playlistManager.tryPlay(playlistIndex, index);
    }

}

void Application::continueWatching() {
    int index = m_showManager.getContinueIndex();
    if (index < 0) index = 0;
    playFromEpisodeList(index, false);
}

void Application::updateTimeStamp() {
    // Update the last play time
    auto currentPlaylist = m_playlistManager.getCurrentPlaylist();
    if (!currentPlaylist || currentPlaylist->currentIndex == -1) return;
    auto time = MpvObject::instance()->time();

    cLog() << "Playlist" << "Saving timestamp" << time << "for" << currentPlaylist->link;

    // if (time > 0.95 * MpvObject::instance()->duration() && currentPlaylist->currentIndex + 1 < currentPlaylist->size()) {
    //     cLog() << "App" << "Setting to next episode" << currentPlaylist->link;
    //     currentPlaylist->setLastPlayAt(currentPlaylist->currentIndex + 1, time);


    currentPlaylist->setLastPlayAt(currentPlaylist->currentIndex, time);


    if (currentPlaylist->isLoadedFromFolder()){
        currentPlaylist->updateHistoryFile(time);
        cLog() << "App" << "Updating history file for" << currentPlaylist->link << "to" << time;
    } else {
        m_libraryManager.updateLastWatchedIndex(currentPlaylist->link, currentPlaylist->currentIndex, time);
    }

    /*else {
        if (currentPlaylist->currentIndex == currentPlaylist->size() - 2 && time > 0.95 * MpvObject::instance()->duration()) {
            currentPlaylist->setLastPlayAt(currentPlaylist->currentIndex, time);
        } else {
            currentPlaylist->setLastPlayAt(currentPlaylist->currentIndex,  ? 0 : time);
        }



    }*/
}

void Application::updateLastWatchedIndex() {
    PlaylistItem *currentPlaylist = m_playlistManager.getCurrentPlaylist();
    if (!currentPlaylist) return;

    auto currentShowPlaylist = m_showManager.getPlaylist();
    if (currentShowPlaylist && currentShowPlaylist->link == currentPlaylist->link)
        m_showManager.updateContinueEpisode();

    m_libraryManager.updateLastWatchedIndex(currentPlaylist->link, currentPlaylist->currentIndex, 0);
}


