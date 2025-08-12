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


Application::Application(const QString &launchPath, QObject *parent) : QObject(parent),
    m_searchManager(this), m_searchResultModel(&m_searchManager),
    m_libraryManager(this), m_libraryModel(&m_libraryManager), m_libraryProxyModel(&m_libraryModel),
    m_playlistManager(this), m_playlistModel(&m_playlistManager)
{
    xmlInitParser();
    QNetworkProxyFactory::setUseSystemConfiguration(true);
    m_libraryProxyModel.setSourceModel(&m_libraryModel);

    // qputenv("HTTP_PROXY", QByteArray("http://127.0.0.1:7897"));
    // qputenv("HTTPS_PROXY", QByteArray("http://127.0.0.1:7897"));

    REGISTER_QML_SINGLETON(Application, this);
    REGISTER_QML_SINGLETON(ErrorDisplayer, &ErrorDisplayer::instance());

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



    // QObject::connect(&m_playlistModel, &PlaylistModel::selectionsChanged, this, &Application::updateLastWatchedIndex);

    QObject::connect(&m_playlistManager, &PlaylistManager::progressUpdated, &m_libraryManager, &LibraryManager::updateProgress);
    QObject::connect(&m_libraryManager, &LibraryManager::fetchedAllEpCounts, &m_libraryProxyModel, &LibraryProxyModel::invalidate);


    QObject::connect(&m_showManager, &ShowManager::lastWatchedIndexChanged,
                     this, [&](){
                         m_libraryManager.updateProgress(m_showManager.getShow().link, m_showManager.getLastWatchedIndex(), 0);
                     });

    QObject::connect(&m_showManager, &ShowManager::showChanged,
                     this, [&](){
                         m_libraryManager.updateShowCover(m_showManager.getShow());
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
        QJsonObject showJson = m_libraryManager.getShowJsonAt(m_libraryProxyModel.mapToAbsoluteIndex(index));
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
    auto shouldReplaceFirst = m_playlistManager.at(0) && m_playlistManager.at(0)->seasonNumber == -1;
    int playlistIndex = shouldReplaceFirst ? m_playlistManager.replace(0, playlist) : m_playlistManager.insert(0, playlist);
    m_playlistManager.tryPlay(playlistIndex);

}

void Application::continueWatching() {
    int index = m_showManager.getContinueIndex();
    if (index < 0) index = 0;
    playFromEpisodeList(index, false);
}

void Application::appendToPlaylists(int index, bool fromLibrary, bool play) {
    auto result = QtConcurrent::run([this, index, fromLibrary, play] {
        ShowData show("", "", "", nullptr); // Dummy
        int lastWatchedIndex = 0;
        int timestamp = 0;

        if (fromLibrary) {
            QJsonObject showJson = m_libraryManager.getShowJsonAt(index);
            if (showJson.isEmpty()) return;

            QString providerName = showJson["provider"].toString();
            ShowProvider *provider = m_providerManager.getProvider(providerName);

            if (!provider) {
                ErrorDisplayer::instance().show(providerName + " does not exist", "Show Error");
                return;
            }

            show = ShowData::fromJson(showJson, provider);
            lastWatchedIndex = showJson["lastWatchedIndex"].toInt(0);
            timestamp = showJson["timeStamp"].toInt(0);
        } else {
            show = m_searchManager.getResultAt(index);
            auto lastWatchedInfo = m_libraryManager.getLastWatchInfo(show.link);
            if (lastWatchedInfo.listType != -1) {
                lastWatchedIndex = lastWatchedInfo.lastWatchedIndex;
                timestamp = lastWatchedInfo.timeStamp;
            }
        }

        // Check if playlist already exists in the playlist manager
        auto playlist = m_playlistManager.find(show.link);
        if (!playlist) {
            Client client(nullptr);
            show.provider->loadDetails(&client, show, false, true, false);
            playlist = show.getPlaylist();
            if (playlist->setCurrentIndex(lastWatchedIndex)) {
                playlist->getCurrentItem()->setTimestamp(timestamp);
            }
        }

        // Returns the index of playlist if already added
        auto playlistIndex = m_playlistManager.append(playlist);

        if (play) {
            m_playlistManager.tryPlay(playlistIndex);
        }
    });
}





