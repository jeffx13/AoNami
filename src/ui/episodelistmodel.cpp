#include "ui/episodelistmodel.h"
#include "media/playlistitem.h"
#include <cmath>

void EpisodeListModel::setPlaylist(const QSharedPointer<PlaylistItem> &playlist) {
    beginResetModel();
    m_playlist = playlist;
    rebuildFilteredIndices();
    endResetModel();
}

void EpisodeListModel::setIsReversed(bool isReversed) {
    if (m_isReversed == isReversed) return;
    beginResetModel();
    m_isReversed = isReversed;
    endResetModel();
    emit isReversedChanged();
}

void EpisodeListModel::setFilterText(const QString &text) {
    if (m_filterText == text) return;
    beginResetModel();
    m_filterText = text;
    rebuildFilteredIndices();
    endResetModel();
    emit filterTextChanged();
}

void EpisodeListModel::rebuildFilteredIndices() {
    m_filteredIndices.clear();
    if (m_filterText.isEmpty()) return;
    auto playlist = m_playlist.toStrongRef();
    if (!playlist) return;
    const int count = playlist->count();
    for (int i = 0; i < count; i++) {
        auto ep = playlist->at(i);
        if (!ep) continue;
        // Match the title OR the episode number (e.g. typing "1166" finds E1166).
        const double n = ep->number;
        const QString numStr = (std::floor(n) == n)
            ? QString::number(static_cast<long long>(n))
            : QString::number(n);
        if (ep->name.contains(m_filterText, Qt::CaseInsensitive) ||
            numStr.contains(m_filterText))
            m_filteredIndices.append(i);
    }
}

int EpisodeListModel::sourceIndex(int visibleRow) const {
    auto playlist = m_playlist.toStrongRef();
    if (!playlist) return visibleRow;
    if (!m_filterText.isEmpty()) {
        int fi = m_isReversed ? m_filteredIndices.size() - 1 - visibleRow : visibleRow;
        return (fi >= 0 && fi < m_filteredIndices.size()) ? m_filteredIndices[fi] : -1;
    }
    int total = playlist->count();
    return m_isReversed ? total - 1 - visibleRow : visibleRow;
}

int EpisodeListModel::visibleIndex(int sourceIdx) const {
    if (sourceIdx < 0) return -1;
    auto playlist = m_playlist.toStrongRef();
    if (!playlist) return sourceIdx;
    if (!m_filterText.isEmpty()) {
        int fi = m_filteredIndices.indexOf(sourceIdx);
        if (fi < 0) return -1;
        return m_isReversed ? m_filteredIndices.size() - 1 - fi : fi;
    }
    int total = playlist->count();
    return m_isReversed ? total - 1 - sourceIdx : sourceIdx;
}

int EpisodeListModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid() || m_playlist.isNull()) return 0;
    if (!m_filterText.isEmpty()) return m_filteredIndices.size();
    auto playlist = m_playlist.toStrongRef();
    return playlist ? playlist->count() : 0;
}

QVariant EpisodeListModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || m_playlist.isNull()) return {};
    auto playlist = m_playlist.toStrongRef();
    if (!playlist) return {};

    const int count = rowCount();
    if (index.row() >= count) return {};

    int i;
    if (!m_filterText.isEmpty()) {
        int fi = m_isReversed ? m_filteredIndices.size() - 1 - index.row() : index.row();
        i = m_filteredIndices[fi];
    } else {
        int total = playlist->count();
        i = m_isReversed ? total - index.row() - 1 : index.row();
    }

    auto episode = playlist->at(i);
    if (!episode) return {};

    switch (role) {
    case TitleRole:         return episode->name;
    case EpisodeNumberRole: return episode->number;
    case SeasonNumberRole:  return episode->season;
    default:                return {};
    }
}

QHash<int, QByteArray> EpisodeListModel::roleNames() const {
    return {
            {TitleRole, "title"},
            {EpisodeNumberRole, "episodeNumber"},
            {SeasonNumberRole, "seasonNumber"},
            };
}
