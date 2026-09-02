#pragma once
#include <QUrl>
#include <QString>
#include <QSharedPointer>
#include <functional>

class PlaylistItem;

namespace LocalMedia {

struct ParsedUrl {
    QUrl url;
    QString raw;
    bool valid = false;
};

// An empty url falls back to the clipboard, which may hold a url or a curl command.
ParsedUrl parse(QUrl url);

bool loadFolder(const QUrl &pathUrl, const QSharedPointer<PlaylistItem> &playlist,
                const std::function<bool(const QString &)> &isRegistered,
                int curDepth = 0, int maxDepth = 1);

}
