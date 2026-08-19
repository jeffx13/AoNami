#pragma once
#include <QList>
#include <QString>

struct DanmakuComment {
    int     timeMs   = 0;
    int     mode     = 1;          // 1/2/3 scroll, 4 bottom, 5 top
    quint32 color    = 0xFFFFFFu;  // RGB
    int     weight   = 10;         // spam score, 1..11
    int     fontSize = 25;         // 18 small, 25 normal, 36 large
    QString text;
};

// Pushed in from Settings: extraction runs on worker threads, which must not
// touch QSettings.
struct DanmakuOptions {
    bool    enabled      = true;
    int     opacityPct   = 80;
    int     fontScalePct = 100;
    int     speedPct     = 100;
    int     areaPct      = 85;
    int     maxLines     = 0;    // 0 = derive from areaPct
    int     minWeight    = 0;    // 0 = off
    int     maxOnScreen  = 60;   // 0 = unlimited

    QString font         = QStringLiteral("Microsoft YaHei");
    bool    bold         = false;
    int     outline      = 1;    // 0 none, 1 outline, 2 outline + shadow

    bool blockScroll = false;
    bool blockTop    = false;
    bool blockBottom = false;
    bool blockColour = false;    // white instead of dropped
    bool blockRepeat = true;

    static DanmakuOptions current();
    static void set(const DanmakuOptions &options);
};

namespace DanmakuAss {

QString writeFile(QList<DanmakuComment> comments, const QString &cacheKey,
                  const DanmakuOptions &options, const QString &outDir);

void pruneCache(const QString &outDir, int maxAgeDays = 7, int maxFiles = 100);

}  // namespace DanmakuAss
