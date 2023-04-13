#ifndef APPLICATIONMODEL_H
#define APPLICATIONMODEL_H

#include "episodelistmodel.h"
#include "playlistmodel.h"
#include "searchresultsmodel.h"

#include <QAbstractListModel>
#include "watchlistmodel.h"

class ApplicationModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(PlaylistModel* playlistModel READ playlistModel CONSTANT)
    Q_PROPERTY(EpisodeListModel* episodeListModel READ episodeListModel CONSTANT)
    Q_PROPERTY(SearchResultsModel* searchResultsModel READ searchResultsModel CONSTANT)
    Q_PROPERTY(WatchListModel* watchList READ watchListModel CONSTANT)

    EpisodeListModel m_episodeListModel;
    PlaylistModel m_playlistModel;
    SearchResultsModel m_searchResultsModel;
    WatchListModel m_watchListModel;
public:
    static ApplicationModel& instance()
    {
        static ApplicationModel s_instance;
        return s_instance;
    }
    Q_INVOKABLE void loadSourceFromList(int index){
        emit loadingStart();
        int watchListIndex = -1;
        if(Global::instance ().currentShowObject ()->isInWatchList ()){
            watchListIndex = m_watchListModel.getIndex (Global::instance ().currentShow ());
        }
        m_playlistModel.syncList (watchListIndex);
        if(m_episodeListModel.getIsReversed()){
            index = Global::instance ().currentShowObject()->episodes().count () - index - 1;
        }
        m_playlistModel.loadSource (index);
        emit loadingEnd ();
    }
    PlaylistModel* playlistModel(){return &m_playlistModel;} ;
    EpisodeListModel* episodeListModel(){return &m_episodeListModel;} ;
    SearchResultsModel* searchResultsModel(){return &m_searchResultsModel;} ;
    WatchListModel* watchListModel(){return &m_watchListModel;} ;
signals:
    void loadingStart();
    void loadingEnd();
private:
    explicit ApplicationModel(QObject *parent = nullptr): QObject(parent){
        connect(&m_watchListModel,&WatchListModel::detailsRequested,&m_searchResultsModel,&SearchResultsModel::getDetails);
        connect(&m_playlistModel,&PlaylistModel::setLastWatchedIndex,&m_watchListModel,&WatchListModel::updateLastWatchedIndex);
        connect(&m_watchListModel,&WatchListModel::indexMoved,&m_playlistModel,&PlaylistModel::changeWatchListIndex);
    };

    ~ApplicationModel() {} // Private destructor to prevent external deletion.
    ApplicationModel(const ApplicationModel&) = delete; // Disable copy constructor.
    ApplicationModel& operator=(const ApplicationModel&) = delete; // Disable copy assignment.

private:
};

#endif // APPLICATIONMODEL_H
