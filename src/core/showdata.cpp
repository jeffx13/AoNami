#include "showdata.h"
#include "player/playlistitem.h"
#include "providers/showprovider.h"

void ShowData::setPlaylist(QSharedPointer<PlaylistItem> playlist) {
    m_playlist = std::move(playlist);
}

void ShowData::addEpisode(int seasonNumber, float number, const QString &link, const QString &name) {
    if (!m_playlist)
        m_playlist = QSharedPointer<PlaylistItem>::create(title, provider, this->link);
    m_playlist->emplaceBack(seasonNumber, number, link, name, false);
}

void ShowData::reserveEpisodes(size_t count) {
    if (!m_playlist)
        m_playlist = QSharedPointer<PlaylistItem>::create(title, provider, this->link);
    m_playlist->reserve(static_cast<int>(count));
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

QString ShowData::toString() const {
    QString s;
    s.reserve(512);
    auto append = [&](const char *label, const QString &value) {
        if (!value.isEmpty())
            s += QLatin1String(label) + value + '\n';
    };

    append("Title: ", title);
    append("Link: ", link);
    append("Cover URL: ", coverUrl);
    append("Provider: ", provider ? provider->name() : QString());
    append("Latest Text: ", latestTxt);
    append("Description: ", description);
    append("Views: ", views);
    append("Score: ", score);
    append("Release Date: ", releaseDate);
    append("Status: ", status);
    append("Genres: ", genres.join(','));
    append("Update Time: ", updateTime);
    return s;
}
