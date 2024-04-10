#include "episodelistmodel.h"



void EpisodeListModel::setPlaylist(PlaylistItem *playlist) {
    if (!playlist) {
        m_rootItem->clear();
    } else {
        if (!m_rootItem->isEmpty()) {
            m_rootItem->replace (0, playlist);
        } else {
            m_rootItem->append(playlist);
            m_rootItem->currentIndex = 0;
        }
    }
    updateLastWatchedIndex();
}

void EpisodeListModel::updateLastWatchedIndex() {
    if (auto playlist = m_rootItem->getCurrentItem(); playlist){
        // If the index in second to last of the latest episode then continue from latest episode
        m_continueIndex = playlist->currentIndex < 0 ? 0 : playlist->currentIndex;
        setIsReversed(playlist->currentIndex > 0);
        const PlaylistItem *episode = playlist->at(m_continueIndex);
        m_continueText = playlist->currentIndex == -1 ? "Play " : "Continue from ";
        m_continueText += episode->name.isEmpty()
                              ? QString::number (episode->number)
                              : episode->number < 0
                                    ? episode->name
                                    : QString::number (episode->number) + "\n" + episode->name;

    } else {
        m_continueIndex = -1;
        m_continueText = "";
    }

    emit continueIndexChanged();
}

int EpisodeListModel::getContinueIndex() const {
    auto playlist = m_rootItem->getCurrentItem();
    if (!playlist) return -1;
    return m_isReversed ? playlist->size() - 1 -  m_continueIndex : m_continueIndex;
}

int EpisodeListModel::correctIndex(int index) const {
    auto currentPlaylist = m_rootItem->getCurrentItem();
    if (!currentPlaylist){
        qFatal() << "Log (Episode List): No current playlist";
        return -1;
    }
    Q_ASSERT(index > -1 && index < currentPlaylist->size());
    return m_isReversed ? currentPlaylist->size() - index - 1: index;
}

int EpisodeListModel::getLastWatchedIndex() const {
    auto currentPlaylist = m_rootItem->getCurrentItem();
    if (!currentPlaylist || currentPlaylist->currentIndex == -1) return -1;
    return correctIndex (currentPlaylist->currentIndex);
}

int EpisodeListModel::rowCount(const QModelIndex &parent) const
{
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
