#include "episodelistmodel.h"
#include "base/player/playlistitem.h"


void EpisodeListModel::setPlaylist(PlaylistItem *playlist) {
    if (m_playlist) m_playlist->disuse();
    if (playlist) playlist->use();
    m_playlist = playlist;
}



int EpisodeListModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid() || !m_playlist)
        return 0;
    return m_playlist->size();
}

QVariant EpisodeListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || !m_playlist)
        return {};
    int i = m_isReversed ? m_playlist->size() - index.row() - 1 : index.row();
    const PlaylistItem* episode = m_playlist->at(i);
    if (!episode) return {};
    switch (role) {
    case TitleRole:
        return episode->name;
    case NumberRole:
        return episode->number;
    case FullTitleRole:
        return episode->getFullName();
    }
    return {};
}

QHash<int, QByteArray> EpisodeListModel::roleNames() const {
    QHash<int, QByteArray> names;
    names[TitleRole] = "title";
    names[NumberRole] = "number";
    names[FullTitleRole] = "fullTitle";
    return names;
}
