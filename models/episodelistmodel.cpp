#include "episodelistmodel.h"

EpisodeListModel::EpisodeListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}


int EpisodeListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return m_episodeList.count ();
}

QVariant EpisodeListModel::data(const QModelIndex &index, int role) const{

    if (!index.isValid())
        return QVariant();
    int i = index.row();
    if(isReversed){
        i = m_episodeList.count () - i - 1;
    }
    const Episode& episode = m_episodeList.at(i);

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


