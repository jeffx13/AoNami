#pragma once
#include <QList>
#include <QString>
#include <QUrl>

class Client;

namespace SubDl {

// SubDL wraps files in a release even when there is only one, so results are flattened.
struct Result {
    QString fileId;        // stable per file; doubles as the cache key
    QString name;
    QString releaseName;
    QString language;
    QString author;
    QUrl    url;
    int     season = 0;
    int     episode = 0;
    qint64  size = 0;      // SubDL reports it exactly; used to spot a half-written cache entry
    bool    hearingImpaired = false;
};

// Key and languages passed in: this runs on a worker thread, which must not touch QSettings.
QList<Result> search(Client *client, const QString &query,
                     const QString &apiKey, const QString &languages);

QString fetch(Client *client, const Result &result);

// Where fetch() puts a file. Deterministic, so a result can be recognised as already downloaded.
QString cachePath(const QString &fileId);

}
