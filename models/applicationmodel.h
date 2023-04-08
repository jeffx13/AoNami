#ifndef APPLICATIONMODEL_H
#define APPLICATIONMODEL_H

#include "episodelistmodel.h"
#include "playlistmodel.h"
#include "searchresultsmodel.h"

#include <QAbstractListModel>
#include <WatchListManager.h>

class ApplicationModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(PlaylistModel* playlistModel READ playlistModel CONSTANT)
    Q_PROPERTY(EpisodeListModel* episodeListModel READ episodeListModel CONSTANT)
    Q_PROPERTY(SearchResultsModel* searchResultsModel READ searchResultsModel CONSTANT)

    EpisodeListModel m_episodeListModel;
    PlaylistModel m_playlistModel{&m_episodeListModel};
    SearchResultsModel m_searchResultsModel;
public:
    static ApplicationModel& instance()
    {
        static ApplicationModel s_instance;
        return s_instance;
    }
    Q_INVOKABLE void loadSourceFromList(int index){
        emit loadingStart();
        m_playlistModel.syncList (Global::instance().getCurrentShowProvider());
        if(m_episodeListModel.getIsReversed()){
            index = m_episodeListModel.list ().count () - index - 1;
        }
        m_playlistModel.loadSource (index);
        emit loadingEnd ();
    }
    PlaylistModel* playlistModel(){return &m_playlistModel;} ;
    EpisodeListModel* episodeListModel(){return &m_episodeListModel;} ;
    SearchResultsModel* searchResultsModel(){return &m_searchResultsModel;} ;
signals:
    void loadingStart();
    void loadingEnd();
private:
    explicit ApplicationModel(QObject *parent = nullptr): QObject(parent){

        connect(&WatchListManager::instance(),&WatchListManager::detailsRequested,&m_searchResultsModel,&SearchResultsModel::getDetails);

        for(auto const& provider:Global::instance ().providers ()){
            connect (provider,&ShowParser::episodesFetched,&this->m_episodeListModel,&EpisodeListModel::setEpisodeList);
        }
    };

    ~ApplicationModel() {} // Private destructor to prevent external deletion.
    ApplicationModel(const ApplicationModel&) = delete; // Disable copy constructor.
    ApplicationModel& operator=(const ApplicationModel&) = delete; // Disable copy assignment.

private:
};

#endif // APPLICATIONMODEL_H
