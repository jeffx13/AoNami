#include "episodelistmodel.h"




int EpisodeListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return Global::instance().currentShowObject ()->episodes().count ();
}

QVariant EpisodeListModel::data(const QModelIndex &index, int role) const{

    if (!index.isValid())
        return QVariant();
    int i = index.row();
    if(isReversed){
        i = Global::instance().currentShowObject ()->episodes().count () - i - 1;
    }
    const Episode& episode = Global::instance().currentShowObject ()->episodes().at (i);

    switch (role) {
    case TitleRole:
        return episode.title.isEmpty () ? QVariant() : episode.title;
    case NumberRole:
        return episode.number;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> EpisodeListModel::roleNames() const{
    QHash<int, QByteArray> names;
    names[TitleRole] = "title";
    names[NumberRole] = "number";
    return names;
}


