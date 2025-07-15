#include "application.h"
#include <QNetworkProxyFactory>
#include <QTextCodec>
#include <libxml2/libxml/parser.h>
#include <QQmlContext>
#include <QFontDatabase>
#include <QQuickStyle>
#include "utils/imagenamfactory.h"


#include "utils/logger.h"
#include "providers/iyf.h"
#include "providers/bilibili.h"
#include "providers/allanime.h"
#include "providers/seedbox.h"

// #include "providers/qqvideo.h"



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
        // new QQVideo(this),
        new AllAnime(this),
        new Bilibili(this),
        new SeedBox(this),
        new IyfProvider(this),
        // new Dm84(this),
        // new Haitu(this),
        // new Tangrenjie(this),
        // new Autoembed(this),
        // new WCOFun(this),

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
                         m_libraryManager.updateLastWatchedIndex(playlist->link, playlist->getCurrentIndex(), 0);
                     });




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
    engine.setNetworkAccessManagerFactory(new ImageNAMFactory);
    engine.load(url);
}

Application::~Application() {
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

void Application::explore(const QString &query, int page, bool isLatest) {
    int type = m_providerManager.getCurrentSearchType();
    auto provider = m_providerManager.getCurrentSearchProvider();
    if (!query.isEmpty()) {
        m_searchResultManager.search(query, page, type, provider);
    } else if (isLatest){
        m_searchResultManager.latest(page, type, provider);
    } else {
        m_searchResultManager.popular(page, type, provider);
    }
    m_lastSearch = [this, query, isLatest, page](bool isReload = false) {
        explore(query, isReload ? page : page + 1);
    };
}

void Application::exploreMore(bool isReload) {
    if (!isReload && !m_searchResultManager.canLoadMore()) return;
    m_lastSearch(isReload);
}

void Application::loadShow(int index, bool fromLibrary) {
    if (fromLibrary) {
        QJsonObject showJson = m_libraryManager.getShowJsonAt(index);
        if (showJson.isEmpty()) return;

        QString providerName = showJson["provider"].toString();
        ShowProvider *provider = m_providerManager.getProvider(providerName);
        ShowData show = ShowData::fromJson(showJson, provider);
        int lastWatchedIndex = showJson["lastWatchedIndex"].toInt();
        int timeStamp = showJson["timeStamp"].toInt(0);
        ShowData::LastWatchInfo lastWatchedInfo{ m_libraryManager.getCurrentListType(), lastWatchedIndex, timeStamp };
        lastWatchedInfo.playlist = m_playlistManager.find(show.link);
        m_showManager.setShow(show, lastWatchedInfo);
        if (!provider)
            ErrorHandler::instance().show(providerName + " does not exist", "Show Error");
    } else {
        ShowData show = m_searchResultManager.at(index);
        ShowData::LastWatchInfo lastWatchedInfo = m_libraryManager.getLastWatchInfo(show.link);
        lastWatchedInfo.playlist = m_playlistManager.find(show.link);
        m_showManager.setShow(show, lastWatchedInfo);
    }
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


    if (append) {
        m_playlistManager.append(showPlaylist);
        showPlaylist->setCurrentIndex(index);
    } else {
        showPlaylist->seasonNumber = -1; // mark this as an online playlist which is always the first playlist
        auto firstPlaylist = m_playlistManager.at(0);
        int playlistIndex = 0;
        if (firstPlaylist && firstPlaylist->seasonNumber == -1) {
            saveTimeStamp();
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

void Application::saveTimeStamp() {
    // Update the last play time
    auto currentPlaylist = m_playlistManager.getCurrentPlaylist();
    if (!currentPlaylist || currentPlaylist->getCurrentIndex() == -1) return;

    auto time = MpvObject::instance()->time();
    cLog() << "Playlist" << "Timestamp =" << time << "for" << currentPlaylist->name;

    // if not last episode and near the end of the video, set to 0
    if (currentPlaylist->getCurrentIndex() != currentPlaylist->size() - 1 && time > 0.96 * MpvObject::instance()->duration()){
        currentPlaylist->getCurrentItem()->timeStamp = 0;
    } else {
        currentPlaylist->getCurrentItem()->timeStamp = time;
    }
    if (!currentPlaylist->isLoadedFromFolder())
        currentPlaylist->updateHistoryFile();

    m_libraryManager.updateLastWatchedIndex(currentPlaylist->link, currentPlaylist->getCurrentIndex(), time);

}

void Application::updateLastWatchedIndex() {
    PlaylistItem *currentPlaylist = m_playlistManager.getCurrentPlaylist();
    if (!currentPlaylist) return;

    auto currentShowPlaylist = m_showManager.getPlaylist();
    if (currentShowPlaylist && currentShowPlaylist->link == currentPlaylist->link)
        m_showManager.updateContinueEpisode();

    m_libraryManager.updateLastWatchedIndex(currentPlaylist->link, currentPlaylist->getCurrentIndex(), 0);
}


