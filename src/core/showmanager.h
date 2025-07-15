#pragma once
#include "network/network.h"
#include "showdata.h"
#include "episodelistmodel.h"

#include <QObject>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>

class ShowManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString           title              READ getTitle                                      NOTIFY showChanged)
    Q_PROPERTY(ShowProvider      *provider          READ getProvider                                   NOTIFY showChanged)
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
    Q_PROPERTY(bool              isLoading          READ isLoading                                     NOTIFY isLoadingChanged)
    // Q_PROPERTY(int               listType           READ getListType                                   NOTIFY listTypeChanged)
    Q_PROPERTY(QString           continueText       READ getContinueText                               NOTIFY lastWatchedIndexChanged)
    Q_PROPERTY(int               lastWatchedIndex   READ getLastWatchedIndex WRITE setLastWatchedIndex NOTIFY lastWatchedIndexChanged)
    Q_PROPERTY(EpisodeListModel  *episodeList       READ episodeListModel    CONSTANT)

    QString getTitle()        const { return m_show.title;}
    QString getCoverUrl()     const { return m_show.coverUrl;}
    QString getDescription()  const { return m_show.description;}
    QString getReleaseDate()  const { return m_show.releaseDate;}
    QString getUpdateTime()   const { return m_show.updateTime;}
    QString getRating()       const { return m_show.score;}
    QString getGenresString() const { return m_show.genres.join (' ');}
    QString getViews()        const { return m_show.views;}
    QString getStatus()       const { return m_show.status;}
    QString getContinueText() const { return m_continueText; }
    // int getListType()         const { return m_show.getListType(); }
    QString getLink()         const { return m_show.link; }
    ShowProvider* getProvider() const;


private:
    inline bool exists() const { return !m_show.link.isEmpty(); }

    ShowData m_show{"", "", "", nullptr};
    EpisodeListModel m_episodeList{this};
    EpisodeListModel *episodeListModel() { return &m_episodeList; }
    QFutureWatcher<void> m_watcher;
    int m_continueIndex = -1;
    QString m_continueText = "";

    bool m_isLoading = false;
    bool isLoading() const { return m_isLoading; }
    void setIsLoading(bool isLoading) {
        if (m_isLoading == isLoading) return;
        m_isLoading = isLoading;
        emit isLoadingChanged();
    }

    void loadShow(const ShowData &show, const ShowData::LastWatchInfo &lastWatchInfo);
    std::atomic<bool> m_isCancelled = false;
    Client m_client { &m_isCancelled };

public:
    explicit ShowManager(QObject *parent = nullptr);
    ~ShowManager() = default;

    ShowData &getShow() { return m_show; }
    void setShow(const ShowData &show, const ShowData::LastWatchInfo &lastWatchInfo);


    void setLastWatchedIndex(int index) {
        auto playlist = m_show.getPlaylist();
        if (!playlist || playlist->getCurrentIndex() == index || !playlist->isValidIndex(index))
            return;
        playlist->setCurrentIndex(index);
        updateContinueEpisode(true);
    }
    void updateContinueEpisode(bool notify = false) {
        if (auto playlist = m_show.getPlaylist(); playlist){
            // If the index in second to last of the latest episode then continue from latest episode
            m_continueIndex = playlist->getCurrentIndex() < 0 ? 0 : playlist->getCurrentIndex();
            const PlaylistItem *episode = playlist->at(m_continueIndex);
            m_continueText = playlist->getCurrentIndex() == -1 ? "Play " : "Continue from ";
            m_continueText += episode->name.isEmpty()
                                  ? QString::number (episode->number)
                                  : episode->number < 0
                                        ? episode->name
                                        : QString::number (episode->number) + "\n" + episode->name;

        } else {
            m_continueIndex = -1;
            m_continueText = "";
        }
        if (notify)
            emit lastWatchedIndexChanged();

    }

    inline PlaylistItem *getPlaylist() const { return m_show.getPlaylist(); }
    // void setListType(int listType);

    Q_INVOKABLE void cancel() {
        if (m_watcher.isRunning()){
            m_isCancelled = true;
        }
    }
    int getContinueIndex() const {
        return m_continueIndex;
    }

    int getLastWatchedIndex() const {
        auto currentPlaylist = m_show.getPlaylist();
        if (!currentPlaylist || currentPlaylist->getCurrentIndex() == -1) return -1;
        return currentPlaylist->getCurrentIndex();;
    }

signals:
    void showChanged();
    // void listTypeChanged();
    void isLoadingChanged();
    void isLoadingFromLibraryChanged();
    void lastWatchedIndexChanged(void);

};
