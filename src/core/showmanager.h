#pragma once
#include "network/network.h"
#include "showdata.h"
#include "episodelistmodel.h"

#include <QObject>
#include <QFutureWatcher>
#include <QtConcurrent>

class ShowManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString title READ getTitle NOTIFY showChanged)
    Q_PROPERTY(QString coverUrl READ getCoverUrl NOTIFY showChanged)
    Q_PROPERTY(QString description READ getDescription NOTIFY showChanged)
    Q_PROPERTY(QString releaseDate READ getReleaseDate NOTIFY showChanged)
    Q_PROPERTY(QString status READ getStatus NOTIFY showChanged)
    Q_PROPERTY(QString updateTime READ getUpdateTime NOTIFY showChanged)
    Q_PROPERTY(QString rating READ getRating NOTIFY showChanged)
    Q_PROPERTY(QString views READ getViews NOTIFY showChanged)
    Q_PROPERTY(QString genresString READ getGenresString NOTIFY showChanged)

    Q_PROPERTY(bool exists READ exists NOTIFY showChanged)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)
    Q_PROPERTY(bool isLoadingFromLibrary READ isLoadingFromLibrary NOTIFY isLoadingFromLibraryChanged)
    Q_PROPERTY(int listType READ getListType NOTIFY listTypeChanged)
    Q_PROPERTY(EpisodeListModel *episodeList READ episodeListModel CONSTANT)

    QString getTitle()        const {return m_show.title;}
    QString getCoverUrl()     const {return m_show.coverUrl;}
    QString getDescription()  const {return m_show.description;}
    QString getReleaseDate()  const {return m_show.releaseDate;}
    QString getUpdateTime()   const {return m_show.updateTime;}
    QString getRating()       const {return m_show.score;}
    QString getGenresString() const {return m_show.genres.join (' ');}
    QString getViews()        const {return m_show.views;}
    QString getStatus()       const {return m_show.status;}


private:
    inline bool exists() const { return !m_show.link.isEmpty(); }

    ShowData m_show{"", "", "", nullptr};
    EpisodeListModel m_episodeList{this};
    EpisodeListModel *episodeListModel() { return &m_episodeList; }
    QFutureWatcher<void> m_watcher;

    bool m_isLoading = false;
    bool isLoading() const { return m_isLoading; }
    void setIsLoading(bool isLoading) {
        if (m_isLoading == isLoading) return;
        m_isLoading = isLoading;
        emit isLoadingChanged();
    }

    void loadShow(const ShowData &show, const ShowData::LastWatchInfo &lastWatchInfo);
    std::atomic<bool> m_isCancelled = false;
    Client m_client{&m_isCancelled};
    bool m_isLoadingFromLibrary = false;
public:
    explicit ShowManager(QObject *parent = nullptr);
    ~ShowManager() = default;

    ShowData &getShow() { return m_show; }
    void setShow(const ShowData &show, const ShowData::LastWatchInfo &lastWatchInfo);
    inline void updateLastWatchedIndex() { m_episodeList.updateLastWatchedIndex(); };
    int correctIndex(int index) const {
        return m_episodeList.correctIndex(index);
    };
    bool isLoadingFromLibrary() const { return m_isLoadingFromLibrary; }
    void setIsLoadingFromLibrary(bool isLoadingFromLibrary) {
        m_isLoadingFromLibrary = isLoadingFromLibrary;
        emit isLoadingFromLibraryChanged();
    }


    inline int getContinueIndex() const { return m_episodeList.getContinueIndex(); };
    inline PlaylistItem *getPlaylist() const { return m_show.getPlaylist(); }

    inline int getListType() const { return m_show.getListType(); }
    void setListType(int listType);

    Q_INVOKABLE void cancel() {
        if (m_watcher.isRunning()){
            m_isCancelled = true;
        }
    }
signals:
    void showChanged();
    void listTypeChanged();
    void isLoadingChanged();
    void isLoadingFromLibraryChanged();
};
