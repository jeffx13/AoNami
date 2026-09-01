#include "providers/subdl.h"
#include "app/exception.h"
#include "app/settings.h"
#include "net/client.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QSaveFile>

namespace {

constexpr const char *kApi = "https://api.subdl.com/api/v1/subtitles";
constexpr const char *kFiles = "https://dl.subdl.com";

QString cacheDir() {
    const QString dir = Settings::getTempDir() + QStringLiteral("/subtitles");
    QDir().mkpath(dir);
    return dir;
}

SubDl::Result fileToResult(const QJsonObject &file, const QJsonObject &release) {
    SubDl::Result r;
    r.fileId          = file["file_n_id"].toString();
    r.name            = file["name"].toString();
    r.releaseName     = release["release_name"].toString();
    r.language        = file["language"].toString();
    r.author          = release["author"].toString();
    r.season          = file["season"].toInt();
    r.episode         = file["episode"].toInt();
    r.size            = file["size"].toInteger();
    r.hearingImpaired = file["hi"].toBool();
    r.url             = QUrl(QLatin1String(kFiles) + file["url"].toString());
    return r;
}

}

QList<SubDl::Result> SubDl::search(Client *client, const QString &query,
                                   const QString &apiKey, const QString &languages) {
    if (apiKey.isEmpty())
        throw AppException("No SubDL API key set. Add subtitles/subdlApiKey to settings.ini.", "Subtitles");

    const QMap<QString, QString> params = {
        {"api_key", apiKey},
        {"film_name", query},
        {"languages", languages},
        {"subs_per_page", "30"},
        {"unpack", "1"},   // yields per-file .srt urls, so no archive to unpack
    };

    const auto response = client->get(kApi, {}, params);
    if (response.code != 200)
        throw AppException(QString("SubDL returned %1.").arg(response.code), "Subtitles");

    const QJsonObject json = response.toJsonObject();
    if (!json["status"].toBool()) {
        const QString error = json["error"].toString();
        throw AppException(error.isEmpty() ? QStringLiteral("SubDL rejected the search.") : error, "Subtitles");
    }

    QList<Result> results;
    const QJsonArray releases = json["subtitles"].toArray();
    for (const QJsonValue &value : releases) {
        const QJsonObject release = value.toObject();
        for (const QJsonValue &file : release["unpack_files"].toArray()) {
            Result r = fileToResult(file.toObject(), release);
            if (!r.fileId.isEmpty() && !r.url.isEmpty())
                results.append(r);
        }
    }
    return results;
}

QString SubDl::fetch(Client *client, const Result &result) {
    const QString path = QDir::cleanPath(cacheDir() + QChar('/') + result.fileId + QStringLiteral(".srt"));
    // A half-written file from an interrupted run is re-fetched, not served.
    if (const qint64 cached = QFileInfo(path).size();
        cached > 0 && (result.size <= 0 || cached == result.size))
        return path;

    const auto response = client->getBytes(result.url.toString());
    if (response.code != 200 || response.bytes.isEmpty())
        throw AppException(QString("Could not download %1.").arg(result.name), "Subtitles");

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        throw AppException("Could not write the subtitle to the cache.", "Subtitles");
    file.write(response.bytes);
    if (!file.commit())
        throw AppException("Could not write the subtitle to the cache.", "Subtitles");
    return path;
}
