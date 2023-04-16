#ifndef EPISODELISTMODEL_H
#define EPISODELISTMODEL_H

#include <Global.h>
#include <QAbstractListModel>

#include <parsers/episode.h>

class EpisodeListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool reversed READ getIsReversed WRITE setIsReversed NOTIFY reversedChanged);
    Q_PROPERTY(QString continueEpisodeName READ getContinueEpisodeName NOTIFY continueIndexChanged);
    Q_PROPERTY(int continueIndex READ getContinueIndex NOTIFY continueIndexChanged);
    enum{
        TitleRole = Qt::UserRole,
        NumberRole
    };
    bool isReversed = false;
    QString continueEpisodeName;
    int continueIndex;
    void setIsReversed(bool isReversed){
        this->isReversed = isReversed;
        emit layoutChanged ();
        emit reversedChanged();
    }

    QString getContinueEpisodeName(){
        return continueEpisodeName;
    }
    int getContinueIndex(){
        return continueIndex;
    }

    void updateLastEpisodeName(){
        continueEpisodeName = "";
        continueIndex = Global::instance().currentShowObject ()->lastWatchedIndex ();
        int totalEpisodes = Global::instance().currentShowObject ()->episodes ().count ();
        if(continueIndex >= 0 && continueIndex < totalEpisodes){
            if(continueIndex==totalEpisodes-2)continueIndex++;
            Episode lastWatchedEpisode = Global::instance().currentShowObject ()->episodes().at (continueIndex);
            continueEpisodeName = QString::number (lastWatchedEpisode.number);
            if(!(lastWatchedEpisode.title.isEmpty () || lastWatchedEpisode.title.toInt () == lastWatchedEpisode.number)){
                continueEpisodeName += "\n" + lastWatchedEpisode.title;
            }
        }
        emit continueIndexChanged();
    }
signals:
    void continueIndexChanged(void);
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
