#ifndef EPISODELISTMODEL_H
#define EPISODELISTMODEL_H

#include <Global.h>
#include <QAbstractListModel>

#include <parsers/episode.h>

class EpisodeListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool reversed READ getIsReversed WRITE setIsReversed CONSTANT);
    Q_PROPERTY(Episode* lastWatchedEpisode READ getLastWatchedEpisode NOTIFY lastWatchedEpisodeChanged);
    QVector<Episode> m_episodeList;
    enum{
        TitleRole = Qt::UserRole,
        NumberRole
    };
    bool isReversed = false;
    Episode lastWatchedEpisode;
    bool noLastEpisode = false;
    void setIsReversed(bool isReversed){
        this->isReversed = isReversed;
        emit layoutChanged ();
    }
    Episode* getLastWatchedEpisode(){
        if(noLastEpisode)return nullptr;
        return &lastWatchedEpisode;
    }

signals:
    void lastWatchedEpisodeChanged(void);

public:
    bool getIsReversed() const{
        return isReversed;
    }
    explicit EpisodeListModel(QObject *parent = nullptr);

    void setEpisodeList(QVector<Episode> newResults){
        m_episodeList = newResults;
        int index = Global::instance().currentShow ().getLastWatchedIndex ();
        if(index >= 0 && index < m_episodeList.count ()){
            lastWatchedEpisode = m_episodeList.at (index);
        }else{
            noLastEpisode = true;
        }
        emit lastWatchedEpisodeChanged();
        emit layoutChanged ();
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;;

    QVector<Episode> list() const{
        return m_episodeList;
    }
    QHash<int, QByteArray> roleNames() const override;;
private:
};

#endif // EPISODELISTMODEL_H
