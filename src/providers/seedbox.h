#pragma once
#include "showprovider.h"
#include "config.h"
class SeedBox : public ShowProvider
{
public:
    explicit SeedBox(QObject *parent = nullptr) : ShowProvider(parent) {
        auto config = Config::get();
        if(config.contains("seedbox_url")) {
            baseUrl = config["seedbox_url"].toString();
        }
        if (config.contains("seedbox_auth")){
            headers["Authorization"] = config["seedbox_auth"].toString();
        }
    }
public:
    QString name() const {return "GigaDrive";};
    QString hostUrl() const {return ""; };
    QList<QString> getAvailableTypes() const { return {"Shows"}; };
    QList<ShowData> search(Client *client, const QString &query, int page, int type) {
        return popular(client, page, type);
    };
    QList<ShowData> popular(Client *client, int page, int typeIndex);;
    QList<ShowData> latest(Client *client, int page, int typeIndex);
    int loadDetails(Client *client, ShowData &show, bool getEpisodeCountOnly, bool fetchPlaylist) const;
    QList<VideoServer> loadServers(Client *client, const PlaylistItem *episode) const { return {VideoServer{"Default", episode->link}}; };
    PlayItem extractSource(Client *client, VideoServer &server);
private:
    QString baseUrl;
    QMap<QString, QString> headers {
        {"Authorization", ""}
    };
};



