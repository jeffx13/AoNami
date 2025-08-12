#include "animepahe.h"

QList<ShowData> AnimePahe::search(Client *client, const QString &query, int page, int type) {
    return {};
}

QList<ShowData> AnimePahe::popular(Client *client, int page, int typeIndex) {
    return {};
}

QList<ShowData> AnimePahe::latest(Client *client, int page, int typeIndex) {
    return {};
}

int AnimePahe::loadDetails(Client *client, ShowData &show, bool getEpisodeCountOnly, bool getPlaylist, bool getInfo) const {
    return {};
}

QList<VideoServer> AnimePahe::loadServers(Client *client, const PlaylistItem *episode) const {
    return {};
}

PlayItem AnimePahe::extractSource(Client *client, VideoServer &server) {
    return {};
}

