#include "showprovider.h"
#include "app/logger.h"
#include "app/settings.h"
#include <QRegularExpression>
#include <QUrl>
#include <algorithm>
#include <numeric>

bool ShowProvider::attachDanmaku(PlayInfo &info, QList<DanmakuComment> comments,
                                 const QString &cacheKey) const {
    const DanmakuOptions options = DanmakuOptions::current();
    if (!options.enabled || comments.isEmpty()) return false;

    const QString path = DanmakuAss::writeFile(comments, cacheKey, options,
                                               Settings::getTempDir() + QStringLiteral("/danmaku"));
    if (path.isEmpty()) return false;

    info.danmaku = std::move(comments);
    info.danmakuKey = cacheKey;

    // Not QUrl(path): a Windows path parses its drive letter as the scheme.
    info.subtitles.emplaceBack(QUrl::fromLocalFile(path), QStringLiteral("弹幕"),
                               QStringLiteral("danmaku"));
    return true;
}

float ShowProvider::resolveTitleNumber(QString &title) const {
    if (title.startsWith(QStringLiteral("第"))) {
        static const QRegularExpression re(QStringLiteral("\\d+"));
        title = re.match(title).captured(0);
    }
    bool ok;
    float number = title.toFloat(&ok);
    if (ok)
        title = QString::number(number);
    return ok ? number : -1.0f;
}
