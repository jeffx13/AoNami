#include "providers/showdata.h"
#include "media/playlistitem.h"
#include "providers/showprovider.h"
#include <QRegularExpression>

void ShowData::setPlaylist(QSharedPointer<PlaylistItem> playlist) {
    m_playlist = std::move(playlist);
}

void ShowData::addEpisode(int seasonNumber, float number, const QString &link, const QString &name, bool preview) {
    if (!m_playlist)
        m_playlist = QSharedPointer<PlaylistItem>::create(title, provider, this->link);
    m_playlist->emplaceBack(seasonNumber, number, link, name, false, preview);
}

void ShowData::addNumberedEpisode(int seasonNumber, const QString &link, QString title) {
    if (title.startsWith(QStringLiteral("第"))) {
        static const QRegularExpression digits(QStringLiteral(R"(\d+)"));
        title = digits.match(title).captured(0);
    }
    bool ok = false;
    const float number = title.toFloat(&ok);
    addEpisode(seasonNumber, ok ? number : -1.0f, link, ok ? QString() : title);
}
