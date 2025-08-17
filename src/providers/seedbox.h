#pragma once
#include "showprovider.h"

class SeedBox : public ShowProvider
{
public:
    explicit SeedBox(QObject *parent = nullptr);
public:
    QString name() const override {return "GigaDrive";};
    QString hostUrl() const override {return ""; };
    QList<QString> getAvailableTypes() const override { return {"Shows"}; };
    QList<ShowData> search(Client *client, const QString &query, int page, int type) override {
        return popular(client, page, type);
    };
    QList<ShowData> popular(Client *client, int page, int typeIndex) override;
    QList<ShowData> latest(Client *client, int page, int typeIndex) override;
    QList<VideoServer> loadServers(Client *client, const PlaylistItem *episode) const override { return {VideoServer{"Default", episode->link}}; };
    PlayInfo extractSource(Client *client, VideoServer &server) override;
private:
    int loadShow(Client *client, ShowData &show, bool getEpisodeCountOnly, bool getPlaylist, bool getInfo) const override;
    QString baseUrl;
    QMap<QString, QString> headers {
        {"Authorization", ""}
    };
};



