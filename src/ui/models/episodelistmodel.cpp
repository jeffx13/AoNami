#include "episodelistmodel.h"
#include "base/player/playlistitem.h"


void EpisodeListModel::setPlaylist(QSharedPointer<PlaylistItem> playlist) {
    m_playlist = QWeakPointer<PlaylistItem>(playlist);
}

void EpisodeListModel::setIsReversed(bool isReversed) {
    if (m_isReversed == isReversed) return;
    m_isReversed = isReversed;
    emit layoutChanged();
    emit isReversedChanged();
}

int EpisodeListModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid() || m_playlist.isNull()) return 0;
    return m_playlist.toStrongRef()->count();
}

QVariant EpisodeListModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || m_playlist.isNull())
        return {};
    auto playlist = m_playlist.toStrongRef();
    int i = m_isReversed ? playlist->count() - index.row() - 1 : index.row();
    auto episode = playlist->at(i);
    if (!episode) return {};
    switch (role) {
    case TitleRole:
        return episode->name;
    case EpisodeNumberRole:
        return episode->number;
    case SeasonNumberRole:
        return episode->season;
    }
    return {};
}

QHash<int, QByteArray> EpisodeListModel::roleNames() const {
    QHash<int, QByteArray> names;
    names[TitleRole] = "title";
    names[EpisodeNumberRole] = "episodeNumber";
    names[SeasonNumberRole] = "seasonNumber";
    return names;
}
