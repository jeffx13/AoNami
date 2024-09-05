#include "episodelistmodel.h"



void EpisodeListModel::setPlaylist(PlaylistItem *playlist) {
    if (!playlist) {
        m_rootItem->clear();
        return;
    }

    if (!m_rootItem->isEmpty()) {
        m_rootItem->replace (0, playlist);
    } else {
        m_rootItem->append(playlist);
        m_rootItem->currentIndex = 0;
    }

}



int EpisodeListModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid())
        return 0;
    auto playlist = m_rootItem->getCurrentItem();
    if (!playlist) return 0;
    return playlist->size();
}

QVariant EpisodeListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    auto playlist = m_rootItem->getCurrentItem();
    if (!playlist) return QVariant();
    int i = m_isReversed ? playlist->size() - index.row() - 1 : index.row();
    const PlaylistItem* episode = playlist->at(i);

    switch (role)
    {
    case TitleRole:
        return episode->name;
    case NumberRole:
        return episode->number;
    case FullTitleRole:
        return episode->getFullName();
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> EpisodeListModel::roleNames() const {
    QHash<int, QByteArray> names;
    names[TitleRole] = "title";
    names[NumberRole] = "number";
    names[FullTitleRole] = "fullTitle";
    return names;
}
