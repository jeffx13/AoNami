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

    auto show = m_playlist.at (index.row ());

    switch (role) {
    case TitleRole:
        return show.title;
        break;
    case NumberRole:
        return show.number;
        break;
    default:
        break;
    }
    return QVariant();
}

QHash<int, QByteArray> PlaylistModel::roleNames() const{
    QHash<int, QByteArray> names;
    names[TitleRole] = "title";
    names[NumberRole] = "number";
    return names;
}
