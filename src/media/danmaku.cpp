#include "media/danmaku.h"
#include "app/logger.h"

#include <QDir>
#include <QMutex>
#include <QSaveFile>
#include <QSet>
#include <QtMath>

#include <algorithm>

namespace {

constexpr int    kResX        = 1920;
constexpr int    kResY        = 1080;
constexpr int    kTopMargin   = 8;
constexpr double kBaseFont    = 48.0;   // px at 1080p
constexpr double kStaticSecs  = 5.0;
constexpr double kScrollSecs  = 10.0;   // at 100% speed
constexpr int    kMaxComments = 10000;
constexpr int    kMaxTextLen  = 100;

QMutex         g_optionsMutex;
DanmakuOptions g_options;

// Laying 45k comments into lanes costs ~40ms; restyling them costs nothing. Appearance
// changes that only touch the [V4+ Styles] line reuse the layout instead of redoing it.
struct Layout {
    QString     key;
    QByteArray  fingerprint;
    QStringList events;
    double      fontPx = 0;
};
QMutex g_layoutMutex;
Layout g_layout;

QByteArray layoutFingerprint(const DanmakuOptions &o, int rawCount) {
    return QByteArray::number(o.fontScalePct) + o.font.toUtf8() + "|"
         + QByteArray::number(o.speedPct)     + "|" + QByteArray::number(o.areaPct)
         + "|" + QByteArray::number(o.maxLines)   + "|" + QByteArray::number(o.minWeight)
         + "|" + QByteArray::number(o.maxOnScreen)
         + "|" + (o.blockScroll ? "1" : "0") + (o.blockTop ? "1" : "0")
         + (o.blockBottom ? "1" : "0") + (o.blockColour ? "1" : "0")
         + (o.blockRepeat ? "1" : "0")
         + "|" + QByteArray::number(rawCount);
}

// 0.55 over-estimates Latin slightly, which is the safe direction for overlap.
bool isWide(char32_t cp) {
    return (cp >= 0x1100 && cp <= 0x115F) || (cp >= 0x2E80 && cp <= 0x303E)
        || (cp >= 0x3041 && cp <= 0x33FF) || (cp >= 0x3400 && cp <= 0x4DBF)
        || (cp >= 0x4E00 && cp <= 0x9FFF) || (cp >= 0xA000 && cp <= 0xA4CF)
        || (cp >= 0xAC00 && cp <= 0xD7A3) || (cp >= 0xF900 && cp <= 0xFAFF)
        || (cp >= 0xFE30 && cp <= 0xFE6F) || (cp >= 0xFF00 && cp <= 0xFF60)
        || (cp >= 0xFFE0 && cp <= 0xFFE6) || (cp >= 0x20000 && cp <= 0x2FFFD)
        || (cp >= 0x30000 && cp <= 0x3FFFD);
}

double textWidth(const QString &text, double fontPx) {
    double em = 0.0;
    for (char32_t cp : text.toUcs4())
        em += isWide(cp) ? 1.0 : 0.55;
    return fontPx * em;
}

// Braces and backslashes would otherwise be read as override tags.
QString sanitize(const QString &raw) {
    QString out;
    out.reserve(raw.size());
    for (QChar ch : raw) {
        const char16_t u = ch.unicode();
        if (u == u'\n' || u == u'\r' || u == u'\t') out += u' ';
        else if (u < 0x20)                          continue;
        else if (u == u'{')                         out += QChar(0xFF5B);
        else if (u == u'}')                         out += QChar(0xFF5D);
        else if (u == u'\\')                        out += QChar(0xFF3C);
        else                                        out += ch;
    }
    return out.simplified();
}

QString assTime(double seconds) {
    if (seconds < 0) seconds = 0;
    int cs = int(qRound(seconds * 100.0));
    const int h = cs / 360000; cs %= 360000;
    const int m = cs / 6000;   cs %= 6000;
    const int s = cs / 100;    cs %= 100;
    return QStringLiteral("%1:%2:%3.%4")
        .arg(h).arg(m, 2, 10, QLatin1Char('0'))
        .arg(s, 2, 10, QLatin1Char('0')).arg(cs, 2, 10, QLatin1Char('0'));
}

struct Lane {
    double lastT        = -1e9;  // last scroller placed here
    double lastW        = 0.0;
    double blockedUntil = -1e9;  // a static comment owns the lane until this time
};

// ASS wants BBGGRR. White falls through to the style, which carries the opacity.
QString colourTag(quint32 rgb) {
    rgb &= 0xFFFFFFu;
    if (rgb == 0xFFFFFFu) return {};
    const quint32 r = (rgb >> 16) & 0xFF, g = (rgb >> 8) & 0xFF, b = rgb & 0xFF;
    // Only the digits get upper-cased: libass matches tag names case-sensitively, and a
    // "\C" is silently ignored, which rendered every coloured comment white.
    const QString hex = QStringLiteral("%1%2%3")
        .arg(b, 2, 16, QLatin1Char('0')).arg(g, 2, 16, QLatin1Char('0'))
        .arg(r, 2, 16, QLatin1Char('0')).toUpper();
    QString tag = QStringLiteral("\\c&H%1&").arg(hex);
    // Dark comments vanish against dark video without a light outline.
    if (0.299 * r + 0.587 * g + 0.114 * b < 60.0)
        tag += QStringLiteral("\\3c&HFFFFFF&");
    return tag;
}

}  // namespace

DanmakuOptions DanmakuOptions::current() {
    QMutexLocker lock(&g_optionsMutex);
    return g_options;
}

void DanmakuOptions::set(const DanmakuOptions &options) {
    QMutexLocker lock(&g_optionsMutex);
    g_options = options;
}

QString DanmakuAss::writeFile(QList<DanmakuComment> comments, const QString &cacheKey,
                              const DanmakuOptions &opt, const QString &outDir) {
    if (!opt.enabled || comments.isEmpty() || outDir.isEmpty()) return {};
    QDir().mkpath(outDir);

    const double fontPx   = kBaseFont * opt.fontScalePct / 100.0;
    const int    laneH    = qMax(1, int(qRound(fontPx * 1.15)));
    const double usableY  = kResY * opt.areaPct / 100.0;
    int laneCount = int((usableY - kTopMargin) / laneH);
    if (opt.maxLines > 0) laneCount = qMin(laneCount, opt.maxLines);
    laneCount = qBound(1, laneCount, 64);
    const double scrollD = kScrollSecs * 100.0 / qMax(1, opt.speedPct);

    // Same path every time, so the player can reload the track it has open.
    const QString path = QStringLiteral("%1/danmaku_%2.ass").arg(outDir, cacheKey);

    const QByteArray fingerprint = layoutFingerprint(opt, int(comments.size()));
    QStringList events;
    {
        QMutexLocker lock(&g_layoutMutex);
        if (g_layout.key == cacheKey && g_layout.fingerprint == fingerprint)
            events = g_layout.events;
    }
    if (events.isEmpty()) {

    int dropMode = 0, dropText = 0, dropDup = 0, dropWeight = 0, dropDensity = 0,
        dropScreen = 0, dropLane = 0;
    const int rawCount = int(comments.size());

    QList<DanmakuComment> kept;
    kept.reserve(comments.size());
    QSet<size_t> seen;
    for (DanmakuComment &c : comments) {
        if (c.mode < 1 || c.mode > 5) { ++dropMode; continue; }
        const bool scrolling = c.mode <= 3;
        if ((scrolling && opt.blockScroll) || (c.mode == 5 && opt.blockTop)
            || (c.mode == 4 && opt.blockBottom)) { ++dropMode; continue; }
        c.text = sanitize(c.text);
        if (c.text.isEmpty() || c.text.size() > kMaxTextLen) { ++dropText; continue; }
        if (opt.blockRepeat) {
            const size_t key = qHash(c.text) ^ (size_t(c.timeMs / 1000) << 1);
            if (seen.contains(key)) { ++dropDup; continue; }
            seen.insert(key);
        }
        if (c.weight < opt.minWeight) { ++dropWeight; continue; }
        if (opt.blockColour) c.color = 0xFFFFFFu;
        kept.append(c);
    }

    // Cap by weight, not by time, so a busy episode keeps its best comments.
    if (kept.size() > kMaxComments) {
        QMap<int, int> hist;
        for (const DanmakuComment &c : kept) ++hist[c.weight];
        int running = 0, threshold = 0;
        for (auto it = hist.constEnd(); it != hist.constBegin(); ) {
            --it;
            running += it.value();
            if (running >= kMaxComments) { threshold = it.key(); break; }
        }
        const int before = int(kept.size());
        QList<DanmakuComment> filtered;
        filtered.reserve(kMaxComments);
        for (const DanmakuComment &c : kept) {
            if (c.weight < threshold) continue;
            filtered.append(c);
            if (filtered.size() >= kMaxComments) break;
        }
        kept = std::move(filtered);
        dropDensity = before - int(kept.size());
    }

    // The greedy first-fit below needs a time-ordered stream.
    std::stable_sort(kept.begin(), kept.end(),
                     [](const DanmakuComment &a, const DanmakuComment &b) {
                         return a.timeMs < b.timeMs;
                     });

    QList<Lane> lanes(laneCount);
    QList<double> active;   // end times of what is currently on screen
    events.reserve(kept.size());

    for (const DanmakuComment &c : kept) {
        const double t = c.timeMs / 1000.0;
        // Never above the base - the lane grid is built from it.
        const double sizeRatio = qBound(0.6, (c.fontSize > 0 ? c.fontSize : 25) / 25.0, 1.0);
        const double glyphPx   = fontPx * sizeRatio;
        const double w = textWidth(c.text, glyphPx);
        const bool scrolling = c.mode <= 3;
        const QString sizeTag = sizeRatio < 0.999
            ? QStringLiteral("\\fs%1").arg(qMax(1, int(qRound(glyphPx))))
            : QString();

        active.removeIf([t](double end) { return end <= t; });
        if (opt.maxOnScreen > 0 && active.size() >= opt.maxOnScreen) { ++dropScreen; continue; }

        int placed = -1;
        if (scrolling) {
            for (int i = 0; i < laneCount; ++i) {
                const Lane &ln = lanes[i];
                if (t < ln.blockedUntil) continue;
                // Previous tail must be fully on screen, and we must not catch
                // it before it exits. The second term needs our own width.
                const double gap = scrollD * qMax(ln.lastW > 0 ? ln.lastW / (kResX + ln.lastW) : 0.0,
                                                  w / (kResX + w));
                if (t >= ln.lastT + gap) { placed = i; break; }
            }
            if (placed < 0) { ++dropLane; continue; }
            lanes[placed].lastT = t;
            lanes[placed].lastW = w;
            const int y = kTopMargin + placed * laneH;
            events += QStringLiteral("Dialogue: 0,%1,%2,Danmaku,,0,0,0,,{%3\\move(%4,%5,%6,%5)}%7")
                .arg(assTime(t), assTime(t + scrollD), colourTag(c.color))
                .arg(kResX).arg(y).arg(-int(qRound(w))).arg(c.text);
        } else {
            const bool top = (c.mode == 5);
            for (int n = 0; n < laneCount; ++n) {
                const int i = top ? n : laneCount - 1 - n;
                const Lane &ln = lanes[i];
                if (t < ln.blockedUntil) continue;
                if (ln.lastT > -1e8 && t < ln.lastT + scrollD) continue;
                placed = i;
                break;
            }
            if (placed < 0) { ++dropLane; continue; }
            lanes[placed].blockedUntil = t + kStaticSecs;
            const int y = top ? kTopMargin + placed * laneH
                              : kTopMargin + (placed + 1) * laneH;
            events += QStringLiteral("Dialogue: 1,%1,%2,Danmaku,,0,0,0,,{%3\\an%4\\pos(%5,%6)}%7")
                .arg(assTime(t), assTime(t + kStaticSecs), colourTag(c.color))
                .arg(top ? 8 : 2).arg(kResX / 2).arg(y).arg(c.text);
        }
        active.append(t + (scrolling ? scrollD : kStaticSecs));
    }

    if (events.isEmpty()) return {};
    gLog() << "Danmaku" << cacheKey << "raw" << rawCount << "kept" << events.size()
           << QStringLiteral("drop{mode=%1,text=%2,dup=%3,weight=%4,density=%5,screen=%6,lane=%7}")
              .arg(dropMode).arg(dropText).arg(dropDup).arg(dropWeight)
              .arg(dropDensity).arg(dropScreen).arg(dropLane)
           << "lanes" << laneCount << "font" << int(fontPx);

    QMutexLocker lock(&g_layoutMutex);
    g_layout = {cacheKey, fingerprint, events, fontPx};
    }

    // mpv counts these as signs - every line carries \move or \pos - and skips style
    // overrides on signs, so sub-scale never reaches the glyphs to be divided back out.
    const int  assFontSize = qMax(1, int(qRound(fontPx)));
    const int  alpha       = qBound(0, 255 - int(qRound(opt.opacityPct / 100.0 * 255.0)), 255);
    const QString alphaHex = QStringLiteral("%1").arg(alpha, 2, 16, QLatin1Char('0')).toUpper();
    // Not arg(): a placeholder before a digit is read as two digits, so
    // "&H%5000000" would mean %50.
    const QString textColour    = QLatin1String("&H") + alphaHex + QLatin1String("FFFFFF");
    const QString outlineColour = QLatin1String("&H") + alphaHex + QLatin1String("000000");

    QString out;
    out.reserve(events.size() * 80 + 1024);
    out += QStringLiteral(
        "[Script Info]\n"
        "ScriptType: v4.00+\n"
        "PlayResX: %1\n"
        "PlayResY: %2\n"
        "WrapStyle: 2\n"
        "ScaledBorderAndShadow: yes\n"
        "YCbCr Matrix: None\n\n"
        "[V4+ Styles]\n"
        "Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, "
        "BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, "
        "BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding\n"
        "Style: Danmaku,%3,%4,%5,%5,%6,%6,%7,0,0,0,100,100,0,0,1,%8,%9,7,0,0,0,1\n\n"
        "[Events]\n"
        "Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text\n")
        .arg(kResX).arg(kResY).arg(opt.font).arg(assFontSize)
        .arg(textColour).arg(outlineColour)
        .arg(opt.bold ? -1 : 0)                        // ASS booleans are 0/-1
        .arg(opt.outline == 0 ? 0 : 1)
        .arg(opt.outline == 2 ? 1 : 0);
    out += events.join(QLatin1Char('\n'));
    out += QLatin1Char('\n');

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        oLog() << "Danmaku" << "Could not write" << path;
        return {};
    }
    file.write("\xEF\xBB\xBF");   // libass sniffs encodings; the BOM removes all doubt
    file.write(out.toUtf8());
    if (!file.commit()) {
        oLog() << "Danmaku" << "Could not commit" << path;
        return {};
    }

    return path;
}

void DanmakuAss::pruneCache(const QString &outDir, int maxAgeDays, int maxFiles) {
    QDir dir(outDir);
    if (!dir.exists()) return;
    const auto files = dir.entryInfoList({QStringLiteral("*.ass")}, QDir::Files, QDir::Time);
    const QDateTime cutoff = QDateTime::currentDateTime().addDays(-maxAgeDays);
    int kept = 0;
    for (const QFileInfo &fi : files) {
        if (++kept > maxFiles || fi.lastModified() < cutoff)
            QFile::remove(fi.absoluteFilePath());
    }
}
