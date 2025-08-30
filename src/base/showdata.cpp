#include "showdata.h"
#include "player/playlistitem.h"
#include "providers/showprovider.h"

void ShowData::setPlaylist(QSharedPointer<PlaylistItem> playlist) {
    m_playlist = playlist;
}

void ShowData::addEpisode(int seasonNumber, float number, const QString &link, const QString &name) {
    if (!m_playlist) {
        m_playlist = QSharedPointer<PlaylistItem>::create(title, provider, this->link);
    }
    m_playlist->emplaceBack(seasonNumber, number, link, name, false);
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
    type = ShowData::NONE;
    m_playlist.reset();
}

void ShowData::reserveEpisodes(size_t count) {
    if (!m_playlist) {
        m_playlist = QSharedPointer<PlaylistItem>::create(title, provider, this->link);
    }
}

QString ShowData::toString() const {
    QStringList stringList;
    stringList.reserve(15);
    
    stringList << "Title: " << title << "\n"
               << "Link: " << link << "\n"
               << "Cover URL: " << coverUrl << "\n"
               << "Provider: " << (provider ? provider->name() : QString()) << "\n";
    
    if (!latestTxt.isEmpty())
        stringList << "Latest Text: " << latestTxt << "\n";
    if (!description.isEmpty())
        stringList << "Description: " << description << "\n";
    if (!views.isEmpty())
        stringList << "Views: " << views << "\n";
    if (!score.isEmpty())
        stringList << "Score: " << score << "\n";
    if (!releaseDate.isEmpty())
        stringList << "Release Date: " << releaseDate << "\n";
    if (!status.isEmpty())
        stringList << "Status: " << status << "\n";
    if (!genres.isEmpty())
        stringList << "Genres: " << genres.join(',') << "\n";
    if (!updateTime.isEmpty())
        stringList << "Update Time: " << updateTime << "\n";

    return stringList.join("");
}
