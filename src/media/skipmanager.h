#pragma once
#include <QObject>
#include <QHash>
#include <QList>
#include <QStringList>
#include <QFutureWatcher>
#include "net/canceltoken.h"
#include <qqmlintegration.h>

class PlaylistItem;
class Client;

// Drives intro/outro skip and the AniSkip panel: resolves the title to a MAL match,
// fetches OP/ED times, applies them to mpv. The match is editable and persists per show.
class SkipManager : public QObject {
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(QString     searchQuery     READ searchQuery     WRITE setSearchQuery     NOTIFY searchQueryChanged)
    Q_PROPERTY(QStringList showTitles      READ showTitles                               NOTIFY candidatesChanged)
    Q_PROPERTY(int         selectedShow    READ selectedShow    WRITE setSelectedShow    NOTIFY selectedShowChanged)
    Q_PROPERTY(int         episodeCount    READ episodeCount                             NOTIFY episodeCountChanged)
    Q_PROPERTY(int         selectedEpisode READ selectedEpisode WRITE setSelectedEpisode NOTIFY selectedEpisodeChanged)
    Q_PROPERTY(QString     status          READ status                                  NOTIFY statusChanged)
    Q_PROPERTY(bool        busy            READ busy                                     NOTIFY statusChanged)
    Q_PROPERTY(QString     introRange      READ introRange                              NOTIFY skipTimesChanged)
    Q_PROPERTY(QString     outroRange      READ outroRange                              NOTIFY skipTimesChanged)
public:
    explicit SkipManager(QObject *parent = nullptr);
    ~SkipManager();

    // Connected to PlaylistManager::currentItemChanged.
    void onCurrentItemChanged(PlaylistItem *item);

    QString searchQuery() const { return m_searchQuery; }
    void setSearchQuery(const QString &q);

    QStringList showTitles() const { return m_showTitles; }
    int selectedShow() const { return m_selectedShow; }
    void setSelectedShow(int i);

    int episodeCount() const { return m_episodeCount; }
    int selectedEpisode() const { return m_selectedEpisode; }
    void setSelectedEpisode(int e);

    QString status() const { return m_status; }
    bool busy() const { return m_busy; }

    // Human-readable AniSkip results for the Skip panel ("1:23 - 2:53", or empty).
    QString introRange() const { return m_introRange; }
    QString outroRange() const { return m_outroRange; }

    Q_INVOKABLE void research();   // re-run the AniList search with the current query

signals:
    void searchQueryChanged();
    void candidatesChanged();
    void selectedShowChanged();
    void episodeCountChanged();
    void selectedEpisodeChanged();
    void statusChanged();
    void skipTimesChanged();

private:
    struct Candidate { int malId = 0; QString title; int episodes = 0; };

    void ensureConnections();
    void onDurationChanged();
    void onAniskipToggled();

    void runSearch();                                       // AniList -> candidates (worker)
    static QList<Candidate> searchCandidates(Client &client, const QString &title);
    void setCandidates(const QList<Candidate> &list, int preferMalId);
    void rebuildEpisodeCount();
    void queryAniSkip();                                    // AniSkip for selection (worker)

    void applyTimes(int opStart, int opEnd, int edStart, int edEnd, int duration);
    void applyReset();
    void clearCandidates();
    void setSkipTimes(int opStart, int opEnd, int edStart, int edEnd);  // updates intro/outroRange
    static QString formatTime(int seconds);
    void setStatus(const QString &text, bool busy);
    int  currentMalId() const;
    bool aniskipEnabled() const;

    void loadProfile(const QString &showLink);
    void saveProfile();
    void loadFallback();   // global manual default OP/ED values
    void saveFallback();
    static QString profileKey(const QString &showLink);

    QString m_showTitle, m_showLink;
    int  m_episode         = -1;
    int  m_playlistEpisodes = 0;
    bool m_isOnline        = false;
    int  m_duration        = 0;

    QString          m_searchQuery;
    QList<Candidate> m_candidates;
    QStringList      m_showTitles;
    int              m_episodeCount    = 0;
    int              m_selectedShow    = -1;
    int              m_selectedEpisode = -1;
    QString          m_status;
    bool             m_busy = false;
    QString          m_introRange, m_outroRange;   // formatted AniSkip results

    bool m_connected = false;
    bool m_applying  = false;   // suppress profile save during programmatic apply

    QHash<QString, int> m_malIdCache;   // showLink -> chosen MAL id
    CancelToken m_searchCancel;
    CancelToken m_skipCancel;
    QFutureWatcher<void> m_searchWatcher;
    QFutureWatcher<void> m_skipWatcher;
};
