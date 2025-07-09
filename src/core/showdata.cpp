#include "showdata.h"
#include "player/playlistitem.h"
#include "providers/showprovider.h"


void ShowData::copyFrom(const ShowData &other) {
    title = other.title;
    link = other.link;
    coverUrl = other.coverUrl;
    latestTxt = other.latestTxt;
    provider = other.provider;
    description = other.description;
    releaseDate = other.releaseDate;
    status = other.status;
    genres = other.genres;
    updateTime = other.updateTime;
    score = other.score;
    views = other.views;
    type = other.type;
    m_listType = other.m_listType;
    m_playlist = other.m_playlist;
    if (m_playlist)
        m_playlist->use();
}

ShowData::ShowData(const ShowData &other) {
    if (this != &other){
        copyFrom (other);
    }
}

ShowData &ShowData::operator=(const ShowData &other) {
    if (this != &other){
        copyFrom (other);
    }
    return *this;
}

ShowData::~ShowData() {
    if (m_playlist) {
        m_playlist->disuse();
    }
}

void ShowData::setPlaylist(PlaylistItem *playlist) {
    if (m_playlist) m_playlist->disuse();
    if (playlist) playlist->use();
    m_playlist = playlist;
}


void ShowData::addEpisode(int seasonNumber, float number, const QString &link, const QString &name) {
    if (!m_playlist) {
        m_playlist = new PlaylistItem(title, provider, this->link);
        m_playlist->use();
    }
    m_playlist->emplaceBack(seasonNumber, number, link, name, false);
}

QJsonObject ShowData::toJsonObject() const {
    QJsonObject object;
    object["title"] = title;
    object["cover"] = coverUrl;
    object["link"] = link;
    object["provider"] = provider->name();
    object["lastWatchedIndex"] = m_playlist ? m_playlist->getCurrentIndex() : -1;
    object["type"] = type;
    return object;
}

QString ShowData::toString() const {
    QStringList stringList;
    stringList << "Title: " << title  << "\n"
               << "Link: " << link << "\n"
               << "Cover URL: " << coverUrl << "\n"
               << "Provider: " << provider->name() << "\n";
    if (!latestTxt.isEmpty()) {
        stringList << "Latest Text: " << latestTxt << "\n";
    }
    if (!description.isEmpty())
    {
        stringList << "Description: " << description << "\n";
    }
    if (!views.isEmpty())
    {
        stringList << "Views: " << views << "\n";
    }
    if (!score.isEmpty())
    {
        stringList << "Score: " << score << "\n";
    }
    if (!releaseDate.isEmpty())
    {
        stringList << "Release Date: " << releaseDate << "\n";
    }
    if (!status.isEmpty())
    {
        stringList << "Status: " << status << "\n";
    }
    if (!genres.isEmpty())
    {
        stringList << "Genres: " << genres.join (',')<< "\n";
    }
    if (!updateTime.isEmpty())
    {
        stringList << "Update Time: " << updateTime << "\n";
    }

    return stringList.join ("");
}


