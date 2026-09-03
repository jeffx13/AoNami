#include "providers/showprovider.h"
#include "app/settings.h"
#include <QUrl>

bool ShowProvider::attachDanmaku(PlayInfo &info, QList<DanmakuComment> comments,
                                 const QString &cacheKey) const {
    const DanmakuOptions options = DanmakuOptions::current();
    if (!options.enabled || comments.isEmpty()) return false;

    const QString path = DanmakuAss::writeFile(comments, cacheKey, options,
                                               Settings::tempDir() + QStringLiteral("/danmaku"));
    if (path.isEmpty()) return false;

    info.danmaku = std::move(comments);
    info.danmakuKey = cacheKey;

    // Not QUrl(path): a Windows path parses its drive letter as the scheme.
    info.subtitles.emplaceBack(QUrl::fromLocalFile(path), QStringLiteral("弹幕"),
                               QStringLiteral("danmaku"));
    return true;
}
