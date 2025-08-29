#pragma once
#include "base/network/network.h"
#include "base/servicemanager.h"
#include "showdata.h"
#include "ui/models/episodelistmodel.h"

#include <QObject>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>

class ShowManager;

class ShowObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString           title              READ getTitle                                      NOTIFY showChanged)
    Q_PROPERTY(QString           coverUrl           READ getCoverUrl                                   NOTIFY showChanged)
    Q_PROPERTY(QString           description        READ getDescription                                NOTIFY showChanged)
    Q_PROPERTY(QString           releaseDate        READ getReleaseDate                                NOTIFY showChanged)
    Q_PROPERTY(QString           status             READ getStatus                                     NOTIFY showChanged)
    Q_PROPERTY(QString           link               READ getLink                                       NOTIFY showChanged)
    Q_PROPERTY(QString           updateTime         READ getUpdateTime                                 NOTIFY showChanged)
    Q_PROPERTY(QString           rating             READ getRating                                     NOTIFY showChanged)
    Q_PROPERTY(QString           views              READ getViews                                      NOTIFY showChanged)
    Q_PROPERTY(QString           genresString       READ getGenresString                               NOTIFY showChanged)
    Q_PROPERTY(bool              exists             READ exists                                        NOTIFY showChanged)
    Q_PROPERTY(ShowProvider      *provider          READ getProvider                                   NOTIFY showChanged)
public:
    explicit ShowObject(QObject *parent = nullptr) : QObject(parent) {};
    ~ShowObject() = default;

    QString       getTitle()            const { return m_show.title;             }
    QString       getCoverUrl()         const { return m_show.coverUrl;          }
    QString       getDescription()      const { return m_show.description;       }
    QString       getReleaseDate()      const { return m_show.releaseDate;       }
    QString       getUpdateTime()       const { return m_show.updateTime;        }
    QString       getRating()           const { return m_show.score;             }
    QString       getGenresString()     const { return m_show.genres.join (' '); }
    QString       getViews()            const { return m_show.views;             }
    QString       getStatus()           const { return m_show.status;            }
    QString       getLink()             const { return m_show.link;              }
    ShowProvider* getProvider()         const { return m_show.provider;          }
    bool          exists()              const { return !m_show.link.isEmpty();   }

    ShowData &getShow() { return m_show; }
    void setShow(const ShowData &show) { m_show = show; emit showChanged(); }
    PlaylistItem *getPlaylist() const { return m_show.getPlaylist(); }
    Q_SIGNAL void showChanged();
private:
    ShowData m_show{"", "", "", nullptr};
};


class ShowManager : public ServiceManager {
    Q_OBJECT
    Q_PROPERTY(EpisodeListModel  *episodeListModel READ getEpisodeListModel CONSTANT)
    Q_PROPERTY(ShowObject        *currentShow      READ getShowObject       CONSTANT)
    Q_PROPERTY(QString           continueText      READ getContinueText                               NOTIFY lastWatchedIndexChanged)
    Q_PROPERTY(int               lastWatchedIndex  READ getLastWatchedIndex WRITE setLastWatchedIndex NOTIFY lastWatchedIndexChanged)
public:
    explicit ShowManager(QObject *parent = nullptr) : ServiceManager(parent) {}
    ~ShowManager() {
        if (m_watcher.isRunning()) {
            m_cancelled = true;
            try { m_watcher.waitForFinished(); } catch(...) {}
        }
    }

    void       setShow(const ShowData &show, const ShowData::LastWatchInfo &lastWatchInfo);
    ShowData   &getShow() { return m_showObject.getShow(); }
    ShowObject *getShowObject() { return &m_showObject; }

    QString getContinueText()  const { return m_continueText; }
    int     getContinueIndex() const { return m_continueIndex; }

    int getLastWatchedIndex() const;
    void setLastWatchedIndex(int index);
    void updateContinueEpisode();
    PlaylistItem *getPlaylist() { return m_showObject.getShow().getPlaylist(); }

    Q_INVOKABLE void cancel();

    Q_SIGNAL void showChanged();
    Q_SIGNAL void lastWatchedIndexChanged();

private:
    EpisodeListModel m_episodeList;
    EpisodeListModel *getEpisodeListModel() { return &m_episodeList; }
    QFutureWatcher<void> m_watcher;
    ShowObject m_showObject { this };
    int m_continueIndex = -1;
    QString m_continueText = "";

    Client m_client { &m_cancelled };
    void loadShow(const ShowData &show, const ShowData::LastWatchInfo &lastWatchInfo);
};
