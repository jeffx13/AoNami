#ifndef EPISODELISTMODEL_H
#define EPISODELISTMODEL_H

#include <Global.h>
#include <QAbstractListModel>

#include <parsers/episode.h>

class EpisodeListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool reversed READ getIsReversed WRITE setIsReversed NOTIFY reversedChanged);
    Q_PROPERTY(QString lastWatchedEpisodeName READ getLastWatchedEpisodeName NOTIFY lastWatchedEpisodeNameChanged);
    enum{
        TitleRole = Qt::UserRole,
        NumberRole
    };
    bool isReversed = false;
    QString lastWatchedEpisodeName;
    void setIsReversed(bool isReversed){
        this->isReversed = isReversed;
        emit layoutChanged ();
        emit reversedChanged();
    }


    QString getLastWatchedEpisodeName(){
        return lastWatchedEpisodeName;
    }

    void updateLastEpisodeName(){
        lastWatchedEpisodeName = "";
        int index = Global::instance().currentShowObject ()->lastWatchedIndex ();
        if(index >= 0 && index < Global::instance().currentShowObject ()->episodes().count ()){
            Episode lastWatchedEpisode = Global::instance().currentShowObject ()->episodes().at (index);
            lastWatchedEpisodeName = QString::number (lastWatchedEpisode.number);
            if(!(lastWatchedEpisode.title.isEmpty () || lastWatchedEpisode.title.toInt () == lastWatchedEpisode.number)){
                lastWatchedEpisodeName += "\n" + lastWatchedEpisode.title;
            }
        }
        emit lastWatchedEpisodeNameChanged();
    }
signals:
    void lastWatchedEpisodeNameChanged(void);
    void reversedChanged(void);
public:
    bool getIsReversed() const{
        return isReversed;
    }
    explicit EpisodeListModel(QObject *parent = nullptr)
        : QAbstractListModel(parent){
        connect(Global::instance ().currentShowObject (), &ShowResponseObject::showChanged,this, [&](){
            updateLastEpisodeName();
            emit layoutChanged();
        });
        connect(Global::instance ().currentShowObject (), &ShowResponseObject::lastWatchedIndexChanged,this, [&](){
            updateLastEpisodeName();
        });
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;;

    QHash<int, QByteArray> roleNames() const override;;
private:
};

#endif // EPISODELISTMODEL_H
