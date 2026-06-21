#include "localfolderloader.h"
#include "playlistitem.h"
#include "app/logger.h"
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>

namespace {
const QStringList kPlayableExtensions{
    "*.mp4", "*.mkv", "*.avi", "*.mp3", "*.flac", "*.wav", "*.ogg", "*.webm", "*.m3u8", "*.mov"
};
}

namespace LocalFolderLoader {

bool load(const QUrl &pathUrl, const QSharedPointer<PlaylistItem> &playlist,
          const std::function<bool(const QString &)> &isRegistered,
          int curDepth, int maxDepth) {
    if (curDepth == maxDepth)
        return true;

    QUrl url = !pathUrl.isEmpty() ? pathUrl : QUrl::fromUserInput(playlist->link);
    if (!url.isValid() || !url.isLocalFile()) return false;

    QFileInfo pathInfo(url.toLocalFile());
    if (!pathInfo.exists()) {
        oLog() << "Playlist" << pathInfo.absoluteFilePath() << "doesn't exist";
        return false;
    }

    QDir playlistDir = pathInfo.isDir() ? QDir(url.toLocalFile()) : pathInfo.dir();
    QFileInfoList fileEntries = playlistDir.entryInfoList(kPlayableExtensions, QDir::Files | QDir::NoDotAndDotDot);
    QFileInfoList dirEntries = playlistDir.entryInfoList(QStringList(), QDir::Dirs | QDir::NoDotAndDotDot);

    playlist->name = playlistDir.dirName();
    playlist->displayName = playlistDir.dirName();
    playlist->link = playlistDir.absolutePath();
    playlist->type |= PlaylistItem::Type::LOCAL;
    playlist->clear();

    if (fileEntries.isEmpty() && dirEntries.isEmpty()) return false;

    playlist->historyFile.reset(new QFile(playlistDir.filePath(".mpv.history")));
    QString fileToPlay;
    int timestamp = 0;

    if (playlist->historyFile->exists()) {
        if (fileEntries.isEmpty()) {
            playlist->historyFile->remove();
        } else if (playlist->historyFile->open(QIODevice::ReadOnly | QIODevice::Text)) {
            auto fileData = QTextStream(playlist->historyFile.data()).readAll().trimmed().split(":");
            playlist->historyFile->close();
            fileToPlay = fileData.first();
            if (fileData.size() == 2)
                timestamp = fileData.last().toInt();
        } else {
            rLog() << "Playlist" << "Failed to open history file";
        }
    }

    // Override history if a specific file was requested
    if (fileEntries.contains(pathInfo) && fileToPlay != pathInfo.fileName()) {
        if (playlist->historyFile->open(QIODevice::WriteOnly | QIODevice::Text)) {
            playlist->historyFile->write(pathInfo.fileName().toUtf8());
            playlist->historyFile->close();
            fileToPlay = pathInfo.fileName();
            timestamp = 0;
        } else {
            rLog() << "Playlist" << "Failed to open and update history file";
        }
    }

    static QRegularExpression fileNameRegex{R"((?:[Ss](?<S>\d{1,2})[Ee](?<E>\d{1,3})[\s\-\.]*| (?<episode>\d{2,3}) ?[\s\-]*)(?<title>[^\(\)]+\w)?.*?\.\w{3,4}$)"};

    for (int i = 0; i < fileEntries.count(); ++i) {
        const QFileInfo &fileInfo = fileEntries[i];
        if (!fileInfo.isFile()) continue;

        QString path = fileInfo.absoluteFilePath();
        QRegularExpressionMatch match = fileNameRegex.match(fileInfo.fileName());

        QString title;
        int season = 0;
        float episodeNumber = -1;
        bool ok;

        if (match.hasMatch()) {
            title = match.hasCaptured("title") ? match.captured("title").simplified() : "";
            season = match.hasCaptured("S") ? match.captured("S").simplified().toInt() : 0;
            QString episodeStr = match.hasCaptured("E")
                                     ? match.captured("E").simplified()
                                     : (match.hasCaptured("episode") ? match.captured("episode").simplified() : "");
            float ep = episodeStr.toFloat(&ok);
            episodeNumber = ok ? ep : i;
        } else {
            title = fileInfo.baseName().simplified();
            float ep = title.toFloat(&ok);
            if (ok) {
                episodeNumber = ep;
                title = "";
            }
        }

        playlist->emplaceBack(season, episodeNumber, path, title, true);

        if (fileInfo.fileName() == fileToPlay) {
            playlist->setCurrentIndex(playlist->count() - 1);
            playlist->last()->setTimestamp(timestamp);
        }
    }

    if (curDepth + 1 < maxDepth) {
        for (const QFileInfo &dirInfo : std::as_const(dirEntries)) {
            QString path = dirInfo.absoluteFilePath();
            if (isRegistered(path)) continue;
            auto subPlaylist = QSharedPointer<PlaylistItem>::create();
            if (load(QUrl::fromLocalFile(path), subPlaylist, isRegistered, curDepth + 1, maxDepth) && !subPlaylist->isEmpty())
                playlist->append(subPlaylist);
        }
    }

    if (playlist->isEmpty()) return false;
    playlist->sort();
    return true;
}

}
