#include "ui/subtitlesearch.h"
#include "app/exception.h"
#include "app/logger.h"
#include "app/settings.h"
#include "media/mpvplayer.h"
#include "net/client.h"

#include <QFileInfo>
#include <QRegularExpression>
#include <QtConcurrent/QtConcurrentRun>

namespace {

// Providers throw, and an exception escaping a worker is std::terminate.
template <typename F>
auto guarded(const CancelToken &cancel, F &&fn) -> decltype(fn()) {
    try {
        return fn();
    } catch (const AppException &e) {
        if (!cancel.isCancelled()) e.show();
        e.print();
    } catch (const std::exception &e) {
        oLog() << "Subtitles" << e.what();
    }
    return {};
}

QStringList releaseTags(const QString &text) {
    constexpr int kMaxTags = 4;   // more than this and the chips crowd out the name
    struct Pattern { QRegularExpression re; const char *suffix; };
    static const Pattern patterns[] = {
        {QRegularExpression(R"(\b(2160p|1440p|1080p|720p|480p)\b)", QRegularExpression::CaseInsensitiveOption), ""},
        {QRegularExpression(R"(\b(BluRay|BDRip|BRRip|WEB-?DL|WEBRip|HDTV|DVDRip|REMUX)\b)", QRegularExpression::CaseInsensitiveOption), ""},
        {QRegularExpression(R"(\b(x265|x264|HEVC|AVC)\b)", QRegularExpression::CaseInsensitiveOption), ""},
        {QRegularExpression(R"(\b(\d{2}(?:\.\d{1,3})?)\s*FPS\b)", QRegularExpression::CaseInsensitiveOption), " fps"},
    };

    QStringList tags;
    for (const Pattern &p : patterns) {
        auto it = p.re.globalMatch(text);
        while (it.hasNext() && tags.size() < kMaxTags) {
            const QString tag = it.next().captured(1) + QLatin1String(p.suffix);
            if (!tags.contains(tag, Qt::CaseInsensitive)) tags.append(tag);
        }
    }
    return tags;
}

QString prettyName(const QString &name) {
    static const QRegularExpression extension(R"(\.(srt|ass|ssa|vtt|sub|txt)$)", QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression langSuffix(R"([.\s_-]+(en|eng|english)$)", QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression separators(R"([._]+)");
    QString s = name;
    s.remove(extension);
    s.remove(langSuffix);
    s.replace(separators, QStringLiteral(" "));
    return s.simplified();
}

}

SubtitleSearch::SubtitleSearch(QObject *parent) : ListModel(parent) {
    connect(&m_searchWatcher, &QFutureWatcher<QList<SubDl::Result>>::finished, this, [this]() {
        if (!m_cancel.isCancelled()) {
            setResults(m_searchWatcher.result());
            m_searchedQuery = m_query;
        }
        m_cancel.reset();
        emit isLoadingChanged();
    });
}

MpvPlayer *SubtitleSearch::mpv() {
    auto *player = MpvPlayer::instance();
    if (player && !m_mpvConnected) {
        m_mpvConnected = true;
        for (auto signal : {&MpvPlayer::primarySubIdChanged, &MpvPlayer::secondarySubIdChanged,
                            &MpvPlayer::externalSubsChanged})
            connect(player, signal, this, &SubtitleSearch::refreshSlots);
    }
    return player;
}

SubtitleSearch::~SubtitleSearch() {
    m_cancel.cancel();
    // The workers capture `this`; without waiting they can touch a destroyed model at shutdown.
    try { m_searchWatcher.waitForFinished(); } catch (...) { qWarning("SubtitleSearch: search threw"); }
    try { m_fetchWatcher.waitForFinished(); }  catch (...) { qWarning("SubtitleSearch: fetch threw"); }
}

int SubtitleSearch::rowForFileId(const QString &fileId) const {
    for (int i = 0; i < m_rows.size(); ++i)
        if (m_rows[i].result.fileId == fileId) return i;
    return -1;
}

int SubtitleSearch::slotFor(const QString &localPath) {
    auto *player = mpv();
    if (!player || localPath.isEmpty()) return 0;
    const qint64 id = player->externalSubId(localPath);
    if (id == 0) return 0;
    return id == player->primarySubId()   ? 1
         : id == player->secondarySubId() ? 2 : 0;
}

void SubtitleSearch::refreshSlots() {
    bool changed = false;
    for (Row &row : m_rows) {
        const int slot = slotFor(row.localPath);
        if (slot != row.slot) { row.slot = slot; changed = true; }
    }
    if (changed)
        emit dataChanged(index(0), index(m_rows.size() - 1), {SlotRole});
}

int SubtitleSearch::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : m_rows.size();
}

QVariant SubtitleSearch::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_rows.size()) return {};
    const Row &row = m_rows[index.row()];
    switch (role) {
    case DisplayNameRole: return row.displayName;
    case ReleaseRole:     return row.result.releaseName;
    case LanguageRole:    return row.result.language;
    case AuthorRole:      return row.result.author;
    case EpisodeRole:     return row.result.episode > 0 ? QString("E%1").arg(row.result.episode) : QString();
    case HiRole:          return row.result.hearingImpaired;
    case TagsRole:        return row.tags;
    case SlotRole:        return row.slot;
    case FetchingRole:    return row.fetching;
    default:              return {};
    }
}

QHash<int, QByteArray> SubtitleSearch::roleNames() const {
    return {{DisplayNameRole, "displayName"}, {ReleaseRole,   "release"},
            {LanguageRole,    "language"},    {AuthorRole,    "author"},
            {EpisodeRole,     "episodeLabel"},{HiRole,        "hearingImpaired"},
            {TagsRole,        "tags"},        {SlotRole,      "slot"},
            {FetchingRole,    "fetching"}};
}

void SubtitleSearch::setResults(const QList<SubDl::Result> &results) {
    QList<Row> rows;
    rows.reserve(results.size());
    for (const SubDl::Result &r : results) {
        Row row{r, prettyName(r.name), releaseTags(r.releaseName + QChar(' ') + r.name)};
        // A result downloaded by an earlier search is still in a slot; without this it would
        // come back looking unpicked, with no row left to unpick it from.
        if (const QString cached = SubDl::cachePath(r.fileId); QFileInfo(cached).size() > 0) {
            row.localPath = cached;
            row.slot = slotFor(cached);
        }
        rows.append(std::move(row));
    }

    beginResetModel();
    m_rows = std::move(rows);
    endResetModel();
    emit countChanged();
}

void SubtitleSearch::search(const QString &query) {
    const QString trimmed = query.trimmed();
    if (trimmed.isEmpty() || m_searchWatcher.isRunning()) return;

    m_query = trimmed;
    emit queryChanged();

    // Read here, on the GUI thread; the worker must not touch QSettings.
    const QString apiKey = Settings::instance().subdlApiKey();
    const QString languages = Settings::instance().subdlLanguages();

    m_cancel.reset();
    m_searchWatcher.setFuture(QtConcurrent::run([this, trimmed, apiKey, languages] {
        Client client(m_cancel);
        return guarded(m_cancel, [&] { return SubDl::search(&client, trimmed, apiKey, languages); });
    }));
    emit isLoadingChanged();
}

void SubtitleSearch::use(int row, bool secondary) {
    if (row < 0 || row >= m_rows.size()) return;
    auto *player = mpv();
    if (!player) return;

    const SubDl::Result result = m_rows[row].result;
    if (!m_rows[row].localPath.isEmpty()) {   // cached: nothing to wait on
        player->useExternalSubtitle(m_rows[row].localPath, result.name, result.language, secondary);
        return;
    }
    if (m_fetchWatcher.isRunning()) return;   // one download at a time

    m_rows[row].fetching = true;
    emit dataChanged(index(row), index(row), {FetchingRole});

    m_fetchWatcher.setFuture(QtConcurrent::run([this, result, secondary] {
        Client client(m_cancel);
        const QString path = guarded(m_cancel, [&] { return SubDl::fetch(&client, result); });
        QMetaObject::invokeMethod(this, [this, result, path, secondary] {
            const int row = rowForFileId(result.fileId);
            if (row < 0) return;   // a new search replaced the list while this was in flight
            m_rows[row].fetching = false;
            m_rows[row].localPath = path;
            emit dataChanged(index(row), index(row), {FetchingRole, SlotRole});
            if (path.isEmpty()) return;
            if (auto *player = mpv())
                player->useExternalSubtitle(path, result.name, result.language, secondary);
        }, Qt::QueuedConnection);
    }));
}

void SubtitleSearch::searchIfNew(const QString &query) {
    if (query.trimmed() != m_searchedQuery) search(query);
}

void SubtitleSearch::cancel() {
    m_cancel.cancel();   // covers a fetch too, which isLoading no longer reports
}
