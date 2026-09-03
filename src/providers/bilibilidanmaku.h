#pragma once
#include "media/danmaku.h"
#include "net/client.h"

namespace BilibiliDanmaku {

// durationMs may be 0 - the fetch then probes until a segment comes back empty.
QList<DanmakuComment> fetchAll(Client *client, qint64 cid, int durationMs,
                               const QMap<QString, QString> &headers,
                               const QString &proxyApi);

}
