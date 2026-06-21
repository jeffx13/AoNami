#pragma once
#include <QHash>
#include <QList>
#include "core/network/network.h"
#include "core/playinfo.h"

class ShowProvider;

class ServerSelector {
public:
    struct Result {
        int index = -1;
        PlayInfo playInfo;
        QHash<QString, PlayInfo> cachedSources;
        bool found() const { return index >= 0; }
    };

    static bool checkVideo(Client *client, PlayInfo &playItem);
    static Result findWorkingServer(Client *client, ShowProvider *provider, QList<VideoServer> &servers);
};