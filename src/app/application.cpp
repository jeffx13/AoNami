#include "application.h"
#include <QNetworkProxyFactory>
#include <QTextCodec>
#include <libxml/parser.h>
#include <QQmlContext>
#include <QFontDatabase>
#include <QQuickStyle>

#include "app/config.h"
#include "providers/iyf.h"
#include "providers/bilibili.h"
#include "providers/allanime.h"
#include "providers/seedbox.h"
// #include "providers/qqvideo.h"


Application::Application(const QString &launchPath) :
    m_searchManager(this), m_searchResultModel(&m_searchManager),
    m_libraryManager(this), m_libraryModel(&m_libraryManager), m_libraryProxyModel(&m_libraryModel),
    m_playlistManager(this), m_playlistModel(&m_playlistManager)
{
    REGISTER_QML_SINGLETON(Application, this);
    REGISTER_QML_SINGLETON(ErrorDisplayer, &ErrorDisplayer::instance());

    xmlInitParser();
    QNetworkProxyFactory::setUseSystemConfiguration(true);
    m_libraryProxyModel.setSourceModel(&m_libraryModel);
    Config::load();

    m_providerManager.setProviders(QList<ShowProvider*>{
        // new QQVideo(this),
        new AllAnime(this),
        new Bilibili(this),
        new SeedBox(this),
        new IyfProvider(this),

    });

    if (!launchPath.isEmpty()) {
        QUrl url = QUrl::fromUserInput(launchPath);
        m_playlistManager.openUrl(url, false);
    }

    QObject::connect(&m_playlistManager, &PlaylistManager::progressUpdated, &m_libraryManager, &LibraryManager::updateProgress);

    QObject::connect(&m_libraryManager, &LibraryManager::fetchedAllEpCounts, &m_libraryProxyModel, &LibraryProxyModel::invalidate);

    QObject::connect(&m_showManager, &ShowManager::showChanged,
                     this, [&](){
                         auto show = m_showManager.getShow();
                         m_libraryManager.updateShowCover(show.link, show.coverUrl);
                     });

    std::setlocale(LC_NUMERIC, "C");
    qputenv("LC_NUMERIC", QByteArrayLiteral("C"));
    QQuickStyle::setStyle("Universal");
}

Application::~Application() {
    xmlCleanupParser();
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
        m_searchManager.search(query, page, type, provider);
    } else if (isLatest){
        m_searchManager.latest(page, type, provider);
    } else {
        m_searchManager.popular(page, type, provider);
    }
    m_lastSearch = [this, query, isLatest, page](bool isReload = false) {
        explore(query, isReload ? page : page + 1);
    };
}

void Application::exploreMore(bool isReload) {
    if (!isReload && !m_searchManager.canLoadMore()) return;
    m_lastSearch(isReload);
}

void Application::loadShow(int index, bool fromLibrary) {
    if (fromLibrary) {
        auto showData = m_libraryManager.getData(index, "*");
        if (showData.isNull()) return;
        QVariantMap showDataMap = showData.toMap();
        QString providerName = showDataMap["provider"].toString();
        ShowProvider *provider = m_providerManager.getProvider(providerName);
        if (!provider) {
            ErrorDisplayer::instance().show(providerName + " does not exist", "Show Error");
            return;
        }
        auto show = ShowData::fromMap(showDataMap);
        show.provider = provider;
        int lastWatchedIndex = showDataMap["last_watched_index"].toInt();
        int timestamp = showDataMap["timestamp"].toInt();


        ShowData::LastWatchInfo lastWatchedInfo{ m_libraryManager.getDisplayLibraryType(), lastWatchedIndex, timestamp };
        lastWatchedInfo.playlist = m_playlistManager.find(show.link);
        m_showManager.setShow(show, lastWatchedInfo);
        if (!provider)
            ErrorDisplayer::instance().show(providerName + " does not exist", "Show Error");
    } else {
        ShowData show = m_searchManager.getResultAt(index);
        ShowData::LastWatchInfo lastWatchedInfo = m_libraryManager.getLastWatchInfo(show.link);
        lastWatchedInfo.playlist = m_playlistManager.find(show.link);
        m_showManager.setShow(show, lastWatchedInfo);
    }
}



void Application::downloadCurrentShow(int startIndex, int endIndex) {
    m_downloadManager.downloadShow (m_showManager.getShow(), startIndex, endIndex);
}

void Application::playFromEpisodeList(int index, bool append) {
    if (m_playlistManager.isLoading()) {
        m_playlistManager.cancel();
        return;
    }

    auto playlist = m_showManager.getPlaylist();
    if (!playlist) return;
    playlist->setCurrentIndex(index);
    if (append) {
        m_playlistManager.append(playlist);
        return;
    }

    playlist->seasonNumber = -1; // Marks this as a special playlist that gets replaced everytime user presses play from episode list
    auto firstPlaylist = m_playlistManager.root()->at(0);
    auto shouldReplaceFirst = firstPlaylist && firstPlaylist->seasonNumber == -1;
    int playlistIndex = shouldReplaceFirst ? m_playlistManager.replace(0, playlist) : m_playlistManager.insert(0, playlist);
    m_playlistManager.tryPlay(playlistIndex);

}

void Application::continueWatching() {
    int index = m_showManager.getContinueIndex();
    if (index < 0) index = 0;
    playFromEpisodeList(index, false);
}

void Application::appendToPlaylists(int index, bool fromLibrary, bool play) {
    QString link, title;
    ShowProvider *provider;
    ShowData::LastWatchInfo lastWatchedInfo;
    if (fromLibrary) {
        auto showData = m_libraryManager.getData(index, "title,link,provider,last_watched_index,timestamp");
        if (showData.isNull()) return;
        QMap<QString, QVariant> showDataMap = showData.toMap();
        provider = m_providerManager.getProvider(showDataMap["provider"].toString());
        if (!provider) {
            ErrorDisplayer::instance().show(showDataMap["provider"].toString() + " does not exist", "Show Error");
            return;
        }
        title = showDataMap["title"].toString();
        link = showDataMap["link"].toString();
        lastWatchedInfo.lastWatchedIndex = showDataMap["last_watched_index"].toInt();
        lastWatchedInfo.timestamp = showDataMap["timestamp"].toInt();
    } else {
        auto show = m_searchManager.getResultAt(index);
        link = show.link;
        title = show.title;
        provider = show.provider;
        lastWatchedInfo = m_libraryManager.getLastWatchInfo(link);
    }


    auto result = QtConcurrent::run([this, title, link, provider, lastWatchedInfo, play]() {
        qDebug() << link;
        auto playlist = m_playlistManager.find(link);
        auto dummy =  ShowData(title, link, "", provider);
        if (!playlist) {
            Client client;
            provider->getPlaylist(&client, dummy);
            playlist = dummy.getPlaylist();
            qDebug() << playlist << (playlist ? playlist->name + QString::number(playlist->getCurrentIndex()) : "");

        }

        if (playlist && lastWatchedInfo.lastWatchedIndex != -1) {
            playlist->setCurrentIndex(lastWatchedInfo.lastWatchedIndex);
            playlist->getCurrentItem()->setTimestamp(lastWatchedInfo.timestamp);
        }
        m_playlistManager.append(playlist);
        if (play)
            m_playlistManager.tryPlay(playlist);
    });
}

void Application::setOneInstance() {
    QSharedMemory shared("62d60669-bb94-4a94-88bb-b964890a7e04");
    if ( !shared.create( 512, QSharedMemory::ReadWrite) )
    {
        qWarning() << "Can't start more than one instance of the application.";
        exit(0);
    }
    else {
        cLog() << "App" << "Application started successfully.";
    }
}




