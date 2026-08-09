#pragma once
#include "core/network/network.h"
#include "core/showdata.h"
#include "providers/showprovider.h"  // Full type needed - ShowDetails has Q_PROPERTY(ShowProvider*)
#include "ui/models/episodelistmodel.h"

#include <QObject>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>
#include <qqmlintegration.h>

class ShowManager;

// QObject wrapper exposing ShowData to QML as one bindable `currentShow`.

class ShowDetails : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(QString      title        READ getTitle       NOTIFY showChanged)
    Q_PROPERTY(QString      coverUrl     READ getCoverUrl    NOTIFY showChanged)
    Q_PROPERTY(QString      description  READ getDescription NOTIFY showChanged)
    Q_PROPERTY(QString      releaseDate  READ getReleaseDate NOTIFY showChanged)
    Q_PROPERTY(QString      status       READ getStatus      NOTIFY showChanged)
    Q_PROPERTY(QString      link         READ getLink        NOTIFY showChanged)
    Q_PROPERTY(QString      updateTime   READ getUpdateTime  NOTIFY showChanged)
    Q_PROPERTY(QString      rating       READ getRating      NOTIFY showChanged)
    Q_PROPERTY(QString      views        READ getViews       NOTIFY showChanged)
    Q_PROPERTY(QString      genresString READ getGenresString NOTIFY showChanged)
    Q_PROPERTY(bool         exists       READ exists         NOTIFY showChanged)
    Q_PROPERTY(ShowProvider *provider    READ getProvider    NOTIFY showChanged)

public:
    explicit ShowDetails(QObject *parent = nullptr) : QObject(parent) {}

    QString       getTitle()        const { return m_show.title; }
    QString       getCoverUrl()     const { return m_show.coverUrl; }
    QString       getDescription()  const { return m_show.description; }
    QString       getReleaseDate()  const { return m_show.releaseDate; }
    QString       getUpdateTime()   const { return m_show.updateTime; }
    QString       getRating()       const { return m_show.score; }
    QString       getGenresString() const { return m_show.genres.join(','); }
    QString       getViews()        const { return m_show.views; }
    QString       getStatus()       const { return m_show.status; }
    QString       getLink()         const { return m_show.link; }
    ShowProvider *getProvider()     const { return m_show.provider; }
    bool          exists()          const { return !m_show.link.isEmpty(); }

    const ShowData &getShow() const { return m_show; }
    ShowData &getShow() { return m_show; }
    void setShow(const ShowData &show) { m_show = show; emit showChanged(); }
    QSharedPointer<PlaylistItem> getPlaylist() const { return m_show.getPlaylist(); }

    Q_SIGNAL void showChanged();

private:
    ShowData m_show{"", "", "", nullptr};
};

class ShowManager : public QObject {
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)
    Q_PROPERTY(EpisodeListModel *episodeListModel READ getEpisodeListModel CONSTANT)
    Q_PROPERTY(ShowDetails       *currentShow      READ getShowDetails       CONSTANT)
    Q_PROPERTY(QString           continueText     READ getContinueText     NOTIFY lastWatchedIndexChanged)
    Q_PROPERTY(int               lastWatchedIndex READ getLastWatchedIndex WRITE setLastWatchedIndex NOTIFY lastWatchedIndexChanged)

public:
    explicit ShowManager(QObject *parent = nullptr) : QObject(parent) {
        connect(&m_watcher, &QFutureWatcher<void>::finished, this, &ShowManager::onLoadFinished);
        connect(&m_watcher, &QFutureWatcher<void>::started,  this, &ShowManager::isLoadingChanged);
        connect(&m_watcher, &QFutureWatcher<void>::finished, this, &ShowManager::isLoadingChanged);
    }
    ~ShowManager() {
        if (m_watcher.isRunning()) {
            m_cancel.cancel();
            try { m_watcher.waitForFinished(); } catch (...) { qWarning("ShowManager: waitForFinished threw"); }
        }
    }

    void setShow(const ShowData &show, const ShowData::LastWatchInfo &lastWatchInfo, bool navigate = true);
    const ShowData &getShow() const { return m_showObject.getShow(); }
    ShowData &getShow() { return m_showObject.getShow(); }
    QSharedPointer<PlaylistItem> getPlaylist() const { return m_showObject.getPlaylist(); }

    QString getContinueText()  const { return m_continueText; }
    int     getContinueIndex() const { return m_continueIndex; }
    int     getLastWatchedIndex() const;
    void    setLastWatchedIndex(int index);
    void    updateContinueEpisode();

    // On episode advance, refresh the InfoPage highlight + continue button.
    void    onPlaybackIndexChanged() { updateContinueEpisode(); emit lastWatchedIndexChanged(); }

    Q_INVOKABLE void cancel();

    bool isLoading() const { return m_watcher.isRunning(); }

    Q_SIGNAL void showChanged();
    Q_SIGNAL void lastWatchedIndexChanged();
    Q_SIGNAL void isLoadingChanged();

private:
    CancelToken m_cancel;   // aborts the in-flight loadShow worker

    ShowDetails       *getShowDetails()       { return &m_showObject; }
    EpisodeListModel *getEpisodeListModel() { return &m_episodeList; }

    void loadShow(const ShowData &show, const ShowData::LastWatchInfo &lastWatchInfo, bool navigate);
    void onLoadFinished();

    EpisodeListModel         m_episodeList;
    QFutureWatcher<void>     m_watcher;
    ShowDetails               m_showObject{this};
    int                      m_continueIndex = -1;
    QString                  m_continueText;

    // Pending request stored when setShow is called mid-load; applied in onLoadFinished.
    ShowData                 m_pendingShow;
    ShowData::LastWatchInfo  m_pendingInfo;
    bool                     m_pendingNavigate = true;
    bool                     m_hasPending = false;
};
