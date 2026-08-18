#pragma once
#include "providers/showprovider.h"
#include <QHash>

class Miruro : public ShowProvider
{
public:
    explicit Miruro(QObject *parent = nullptr) : ShowProvider(parent) {}
    QString name() const override { return "Miruro"; }
    QString hostUrl() const override { return "https://www.miruro.to/"; }
    QList<QString> getAvailableTypes() const override { return {"Anime"}; }

    QList<ShowData>    search       (Client *client, const QString &query, int page, int typeIndex) override;
    QList<ShowData>    popular      (Client *client, int page, int typeIndex) override;
    QList<ShowData>    latest       (Client *client, int page, int typeIndex) override;
    QList<VideoServer> loadServers  (Client *client, const PlaylistItem *episode) const override;
    PlayInfo           extractSource(Client *client, VideoServer server) override;

private:
    int loadShow(Client *client, ShowData &show, bool getEpisodeCountOnly, bool getPlaylist, bool getInfo = true) const override;

    // One route for everything: envelope out base64url'd in ?e=, reply XOR'd and gzipped.
    QJsonDocument pipe(Client *client, const QString &path, const QJsonObject &query = {}) const;
    QJsonArray    browse(Client *client, const QJsonObject &query, QList<ShowData> &out) const;

    // Key ships in env2.js and rotates; refetched whenever a reply fails to decode.
    QByteArray obfuscationKey(Client *client, bool refresh = false) const;

    // One payload covering every provider, kept between the playlist build and loadServers.
    QJsonObject episodesFor(Client *client, int anilistId) const;

    mutable QMutex m_cacheMutex;
    mutable QByteArray m_key;
    mutable QHash<int, QJsonObject> m_episodes;
};
