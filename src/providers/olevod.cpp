#include "providers/olevod.h"
#include <QCryptographicHash>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QUrl>


QString Olevod::vv(qint64 unixSeconds) {
    const QString ts = QString::number(unixSeconds);
    QString bits[4];
    for (const QChar &c : ts) {
        const QString b = QString::number(c.unicode(), 2);
        for (int i = 0; i < 4; ++i)
            bits[i] += (i < 3) ? b.mid(2 + i, 1) : b.mid(5);
    }
    QString groups[4];
    for (int i = 0; i < 4; ++i) {
        const QString hex = bits[i].isEmpty() ? QString()
                                              : QString::number(bits[i].toULongLong(nullptr, 2), 16);
        groups[i] = hex.isEmpty() ? QStringLiteral("000") : hex.rightJustified(3, '0').left(3);
    }
    const QString n = QString::fromLatin1(
        QCryptographicHash::hash(ts.toUtf8(), QCryptographicHash::Md5).toHex());
    return n.mid(0, 3) + groups[0] + n.mid(6, 5) + groups[1] + n.mid(14, 5) + groups[2]
         + n.mid(22, 5) + groups[3] + n.mid(30);
}

QString Olevod::signedUrl(const QString &path) {
    return QString("%1%2?_vv=%3").arg(kApi, path, vv(QDateTime::currentSecsSinceEpoch()));
}

QList<ShowData> Olevod::parseList(const QJsonArray &items) {
    QList<ShowData> shows;
    shows.reserve(items.size());
    for (const QJsonValue &v : items) {
        const QJsonObject o = v.toObject();
        const QString id = QString::number(o.value("id").toInt());
        const QString title = o.value("name").toString();
        if (id == "0" || title.isEmpty()) continue;
        QString cover = o.value("picThumb").toString();
        if (cover.isEmpty()) cover = o.value("pic").toString();
        if (!cover.isEmpty() && !cover.startsWith("http")) cover = kStatic + cover;
        shows.emplaceBack(title, id, cover, this, o.value("remarks").toString());
    }
    return shows;
}

QList<ShowData> Olevod::listing(Client *client, int page, int typeIndex, const QString &sort) {
    const int typeId1 = kTypeIds[qBound(0, typeIndex, int(std::size(kTypeIds)) - 1)];
    const QString path = QString("/v1/pub/vod/list/true/3/0/0/%1/0/0/%2/%3/%4")
                             .arg(typeId1).arg(sort).arg(qMax(1, page)).arg(kPageSize);
    return parseList(client->get(signedUrl(path), m_headers)
                         .toJsonObject().value("data").toObject()
                         .value("list").toArray());
}

QList<ShowData> Olevod::search(Client *client, const QString &query, int page, int /*typeIndex*/) {
    if (query.trimmed().isEmpty()) return {};
    const QString path = QString("/v1/pub/index/search/%1/0/0/%2/0")
                             .arg(QString::fromUtf8(QUrl::toPercentEncoding(query)))
                             .arg(qMax(1, page));
    const QJsonArray buckets = client->get(signedUrl(path), m_headers)
                                   .toJsonObject().value("data").toObject()
                                   .value("data").toArray();
    for (const QJsonValue &b : buckets) {
        const QJsonObject o = b.toObject();
        if (o.value("type").toString() == "vod")
            return parseList(o.value("list").toArray());
    }
    return {};
}

QList<ShowData> Olevod::popular(Client *client, int page, int typeIndex) {
    return listing(client, page, typeIndex, QStringLiteral("hot"));
}

QList<ShowData> Olevod::latest(Client *client, int page, int typeIndex) {
    return listing(client, page, typeIndex, QStringLiteral("update"));
}

int Olevod::loadShow(Client *client, ShowData &show, LoadParts parts) const {
    const QString path = QString("/v1/pub/vod/detail/%1/true").arg(show.link);
    const QJsonObject data = client->get(signedUrl(path), m_headers)
                                 .toJsonObject().value("data").toObject();
    if (data.isEmpty()) return 0;

    const QJsonArray urls = data.value("urls").toArray();
    if (parts.testFlag(CountOnly)) return urls.size();

    if (parts.testFlag(Episodes)) {
        for (const QJsonValue &v : urls) {
            const QJsonObject e = v.toObject();
            const QString link = e.value("url").toString();
            if (link.isEmpty()) continue;
            show.addNumberedEpisode(0, link, e.value("title").toString());
        }
    }

    if (parts.testFlag(Details)) {
        show.description = data.value("content").toString();
        show.releaseDate = data.value("year").toVariant().toString();
        show.status      = data.value("remarks").toString();
        show.updateTime  = data.value("vodTime").toVariant().toString();
        show.score       = QString::number(data.value("score").toDouble());
        show.views       = QString::number(data.value("hits").toInt());
        for (const QString &key : {"typeIdName", "area", "lang"}) {
            const QString g = data.value(key).toString();
            if (!g.isEmpty()) show.genres.push_back(g);
        }
    }
    return urls.size();
}

QList<VideoServer> Olevod::loadServers(Client * /*client*/, const PlaylistItem *episode) const {
    return { VideoServer("OleVod", episode->link) };
}

PlayInfo Olevod::extractSource(Client * /*client*/, VideoServer server) {
    PlayInfo info;
    if (server.link.isEmpty()) return info;
    info.videos.emplaceBack(QUrl(server.link));
    info.addHeader("Referer", hostUrl());
    info.addHeader("Origin", "https://www.olevod.com");
    info.addHeader("User-Agent", m_headers.value("User-Agent"));
    return info;
}
