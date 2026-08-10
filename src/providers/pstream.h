#pragma once
#include "providers/showprovider.h"

class PStream : public ShowProvider
{
public:
    explicit PStream(QObject *parent = nullptr);
    QString name() const override { return "P-Stream"; }
    QString hostUrl() const override { return "https://aether.bar/"; }
    QList<QString> getAvailableTypes() const override { return {"Movies", "TV Shows"}; }

    QList<ShowData>    search       (Client *client, const QString &query, int page, int typeIndex) override;
    QList<ShowData>    popular      (Client *client, int page, int typeIndex) override;
    QList<ShowData>    latest       (Client *client, int page, int typeIndex) override;
    QList<VideoServer> loadServers  (Client *client, const PlaylistItem *episode) const override;
    PlayInfo           extractSource(Client *client, VideoServer server) override;

private:
    int loadShow(Client *client, ShowData &show, bool getEpisodeCountOnly, bool getPlaylist, bool getInfo = true) const override;

    QJsonObject tmdb(Client *client, const QString &path, QMap<QString, QString> params = {}) const;
    QList<ShowData> collect(const QJsonArray &results, const QString &kind, int showType) const;

    QMap<QString, QString> m_headers;   // aether checks Origin/Referer, including on the m3u8 proxy
    QString m_token;
};
