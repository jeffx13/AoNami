#include "playlistmodel.h"




int PlaylistModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_playlist.count ();
}

QVariant PlaylistModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    Episode episode = m_playlist[index.row()];


    switch (role) {
    case TitleRole:
        return episode.title;
        break;
    case NumberRole:
        return episode.number;
        break;
    case NumberTitleRole:{
        QString lastWatchedEpisodeName = QString::number (episode.number);
        if(!(episode.title.isEmpty () || episode.title.toInt () == episode.number)){
            lastWatchedEpisodeName += "\n" + episode.title;
        }
        return lastWatchedEpisodeName;
        break;
    }
    default:
        break;
    }
    return QVariant();
}

QHash<int, QByteArray> PlaylistModel::roleNames() const{
    QHash<int, QByteArray> names;
    names[TitleRole] = "title";
    names[NumberRole] = "number";
    names[NumberTitleRole] = "numberTitle";
    return names;
}
