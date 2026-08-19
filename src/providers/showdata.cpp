#include "providers/showdata.h"
#include "media/playlistitem.h"
#include "providers/showprovider.h"

void ShowData::setPlaylist(QSharedPointer<PlaylistItem> playlist) {
    m_playlist = std::move(playlist);
}

void ShowData::addEpisode(int seasonNumber, float number, const QString &link, const QString &name, bool preview) {
    if (!m_playlist)
        m_playlist = QSharedPointer<PlaylistItem>::create(title, provider, this->link);
    m_playlist->emplaceBack(seasonNumber, number, link, name, false, preview);
}

void ShowData::clear() {
    title.clear();
    link.clear();
    coverUrl.clear();
    latestTxt.clear();
    provider = nullptr;
    description.clear();
    releaseDate.clear();
    status.clear();
    genres.clear();
    updateTime.clear();
    score.clear();
    views.clear();
    type = NONE;
    m_playlist.reset();
}
