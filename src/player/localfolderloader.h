#pragma once
#include <QUrl>
#include <QSharedPointer>
#include <functional>

class PlaylistItem;

// Builds a LIST PlaylistItem from a local folder, restoring .mpv.history.
namespace LocalFolderLoader {

bool load(const QUrl &pathUrl, const QSharedPointer<PlaylistItem> &playlist,
          const std::function<bool(const QString &)> &isRegistered,
          int curDepth = 0, int maxDepth = 1);

}
