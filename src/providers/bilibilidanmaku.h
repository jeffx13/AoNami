#pragma once
#include "core/danmaku.h"
#include "core/network/network.h"

namespace BilibiliDanmaku {

QList<DanmakuComment> parseSegment(const QByteArray &data);

// durationMs may be 0 - the fetch then probes until a segment comes back empty.
QList<DanmakuComment> fetchAll(Client *client, qint64 cid, int durationMs,
                               const QMap<QString, QString> &headers,
                               const QString &proxyApi);

}  // namespace BilibiliDanmaku
