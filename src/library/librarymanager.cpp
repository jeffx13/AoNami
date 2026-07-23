#include "librarymanager.h"
#include "providers/providermanager.h"
#include "providers/showprovider.h"
#include "core/network/network.h"
#include <QDir>
#include <QCoreApplication>
#include <QVariant>
#include <QDateTime>
#include <algorithm>

LibraryManager::LibraryManager(QObject *parent)
    : ListModel(parent)
{
    initDatabase();
    refreshDisplayCache();
    connect(&m_fetchWatcher, &QFutureWatcher<void>::finished, this, [this]() {
        m_cancel.reset();
        if (m_pendingFetchLibraryType >= 0) {
            int lt = m_pendingFetchLibraryType;
            bool forced = m_pendingFetchForced;
            m_pendingFetchLibraryType = k_noPendingFetch;
            m_pendingFetchForced = false;
            fetchUnwatchedEpisodes(lt, forced);
        }
    });
}

int LibraryManager::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : m_displayCache.size();
}

QVariant LibraryManager::data(const QModelIndex &index, int role) const {
    if (index.row() < 0 || index.row() >= m_displayCache.size()) return {};
    const LibraryEntry &e = m_displayCache[index.row()];
    using namespace LibraryRoles;
    switch (role) {
    case TitleRole: return e.title;
    case CoverRole: return e.cover;
    case UnwatchedEpisodesRole: {
        if (e.totalEpisodes <= 0) return -1;   // -1 = count unknown, distinct from 0 (caught up)
        // Episodes after the last watched; the latest one counts as unwatched until finished.
        int unwatched = e.totalEpisodes - e.lastWatchedIndex - 1;
        if (!e.finished && e.lastWatchedIndex == e.totalEpisodes - 1)
            unwatched += 1;
        return qMax(0, unwatched);
    }
    case TypeRole: return e.showType;
    default: return {};
    }
}

QHash<int, QByteArray> LibraryManager::roleNames() const {
    using namespace LibraryRoles;
    return {
        {TitleRole, "title"}, {CoverRole, "cover"},
        {UnwatchedEpisodesRole, "unwatchedEpisodes"}, {TypeRole, "type"},
    };
}

LibraryManager::~LibraryManager() {
    disconnect(&m_fetchWatcher, nullptr, this, nullptr);
    if (m_fetchWatcher.isRunning()) {
        m_cancel.cancel();
        try { m_fetchWatcher.waitForFinished(); } catch (...) { qWarning("LibraryManager: waitForFinished threw"); }
    }
    m_db.close();
    m_db = QSqlDatabase();
    QSqlDatabase::removeDatabase(QStringLiteral("AoNami_Library"));
}

void LibraryManager::initDatabase() {
    QString dbPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/library.db");
    m_db = QSqlDatabase::addDatabase("QSQLITE", QStringLiteral("AoNami_Library"));
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        rLog() << "Library" << "Failed to open SQLite DB:" << m_db.lastError().text();
        return;
    }

    QSqlQuery query(m_db);
    // WAL + relaxed sync: faster writes, fine for a media library.
    query.exec("PRAGMA journal_mode=WAL");
    query.exec("PRAGMA synchronous=NORMAL");
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS shows (
            link TEXT PRIMARY KEY,
            title TEXT,
            cover TEXT,
            provider TEXT,
            library_type INTEGER,
            last_watched_index INTEGER,
            timestamp INTEGER,
            total_episodes INTEGER,
            sort_order INTEGER,
            show_type INTEGER DEFAULT 0,
            finished INTEGER DEFAULT 0
        )
    )")) {
        rLog() << "Library" << "Failed to create table:" << query.lastError().text();
        return;
    }

    // Migration: add show_type column if it doesn't exist (for existing DBs)
    query.exec("ALTER TABLE shows ADD COLUMN show_type INTEGER DEFAULT 0");

    // Migration: add "finished"; seed existing rows so caught-up shows read as watched.
    {
        bool hasFinished = false;
        QSqlQuery cols(m_db);
        if (cols.exec("PRAGMA table_info(shows)"))
            while (cols.next())
                if (cols.value(1).toString() == "finished") { hasFinished = true; break; }
        if (!hasFinished) {
            query.exec("ALTER TABLE shows ADD COLUMN finished INTEGER DEFAULT 0");
            query.exec("UPDATE shows SET finished = 1 WHERE last_watched_index >= 0");
        }
    }

    // Watch history is independent of the library so shows you never add still get tracked.
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS history (
            link TEXT PRIMARY KEY,
            title TEXT,
            cover TEXT,
            provider TEXT,
            last_watched_index INTEGER,
            timestamp INTEGER,
            total_episodes INTEGER,
            last_played_at INTEGER DEFAULT 0
        )
    )");

    // Index the hot query: refreshDisplayCache filters by library_type, orders by sort_order.
    query.exec("CREATE INDEX IF NOT EXISTS idx_shows_library ON shows(library_type, sort_order)");
}

LibraryManager::LibraryEntry LibraryManager::entryFromQuery(const QSqlQuery &query) {
    LibraryEntry entry;
    entry.link             = query.value(0).toString();
    entry.title            = query.value(1).toString();
    entry.cover            = query.value(2).toString();
    entry.provider         = query.value(3).toString();
    entry.libraryType      = query.value(4).toInt();
    entry.lastWatchedIndex = query.value(5).toInt();
    entry.timestamp        = query.value(6).toInt();
    entry.totalEpisodes    = query.value(7).toInt();
    entry.showType         = query.value(8).toInt();
    entry.finished         = query.value(9).toInt() != 0;
    entry.valid            = true;
    return entry;
}

void LibraryManager::refreshDisplayCache() {
    m_displayCache.clear();
    QSqlQuery query(m_db);
    query.prepare("SELECT link, title, cover, provider, library_type, last_watched_index, timestamp, total_episodes, show_type, finished "
                  "FROM shows WHERE library_type = ? ORDER BY sort_order");
    query.addBindValue(m_displayLibraryType);
    if (!query.exec()) {
        rLog() << "Library" << "Failed to load display cache:" << query.lastError().text();
        return;
    }
    while (query.next())
        m_displayCache.push_back(entryFromQuery(query));
}

LibraryManager::LibraryEntry LibraryManager::entryForLink(const QString &link) const {
    QSqlQuery query(m_db);
    query.prepare("SELECT link, title, cover, provider, library_type, last_watched_index, timestamp, total_episodes, show_type, finished "
                  "FROM shows WHERE link = ?");
    query.addBindValue(link);
    if (query.exec() && query.next())
        return entryFromQuery(query);
    return {};
}

QVariantList LibraryManager::continueWatching() const {
    QVariantList list;
    QSqlQuery query(m_db);
    query.prepare("SELECT link, title, cover, last_watched_index, total_episodes "
                  "FROM shows WHERE library_type = ? ORDER BY sort_order");
    query.addBindValue(WATCHING);
    if (!query.exec()) return list;
    while (query.next()) {
        const int lwi   = query.value(3).toInt();
        const int total = query.value(4).toInt();
        QVariantMap m;
        m["link"]     = query.value(0).toString();
        m["title"]    = query.value(1).toString();
        m["cover"]    = query.value(2).toString();
        m["progress"] = (total > 0 && lwi >= 0) ? qBound(0.0, double(lwi + 1) / total, 1.0) : 0.0;
        list.append(m);
    }
    return list;
}

QVariantList LibraryManager::history() const {
    QVariantList list;
    QSqlQuery query(m_db);
    query.prepare("SELECT link, title, cover, last_watched_index, total_episodes "
                  "FROM history ORDER BY last_played_at DESC LIMIT 100");
    if (!query.exec()) return list;
    while (query.next()) {
        const int lwi   = query.value(3).toInt();
        const int total = query.value(4).toInt();
        QVariantMap m;
        m["link"]     = query.value(0).toString();
        m["title"]    = query.value(1).toString();
        m["cover"]    = query.value(2).toString();
        m["episode"]  = lwi >= 0 ? lwi + 1 : 0;
        m["total"]    = total;
        m["progress"] = (total > 0 && lwi >= 0) ? qBound(0.0, double(lwi + 1) / total, 1.0) : 0.0;
        list.append(m);
    }
    return list;
}

int LibraryManager::indexOf(const QString &link) {
    if (link.isEmpty()) return -1;
    for (int i = 0; i < m_displayCache.size(); ++i)
        if (m_displayCache[i].link == link)
            return i;
    return -1;
}

bool LibraryManager::add(const ShowData& show, int libraryType) {
    QSqlQuery check(m_db);
    check.prepare("SELECT library_type FROM shows WHERE link = ?");
    check.addBindValue(show.link);
    if (!check.exec()) {
        rLog() << "Library" << "DB check error:" << check.lastError().text();
        return false;
    }

    if (check.next()) {
        int oldLibraryType = check.value(0).toInt();
        if (oldLibraryType == libraryType) {
            return true;
        }
        changeLibraryType(show.link, libraryType);
    } else {
        auto playlist = show.getPlaylist();
        auto lastWatchedIndex = playlist ? playlist->getCurrentIndex() : -1;
        auto totalEpisodes = playlist ? playlist->count() : 0;
        auto currentItem = (lastWatchedIndex != -1 && playlist) ? playlist->getCurrentItem() : nullptr;
        auto timestamp = currentItem ? currentItem->getTimestamp() : 0;

        bool affectsDisplay = (libraryType == m_displayLibraryType);

        QSqlQuery insert(m_db);
        m_db.transaction();
        const int finished = (lastWatchedIndex >= 0) ? 1 : 0;   // existing progress -> treat current as watched
        insert.prepare(R"(
            INSERT INTO shows (link, title, provider, cover, library_type, last_watched_index, timestamp, total_episodes, show_type, finished, sort_order)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, (
                SELECT IFNULL(MAX(sort_order), -1) + 1 FROM shows WHERE library_type = ?
            ))
        )");
        insert.addBindValue(show.link);
        insert.addBindValue(show.title);
        insert.addBindValue(show.provider ? show.provider->name() : QString());
        insert.addBindValue(show.coverUrl);
        insert.addBindValue(libraryType);
        insert.addBindValue(lastWatchedIndex);
        insert.addBindValue(timestamp);
        insert.addBindValue(totalEpisodes);
        insert.addBindValue(show.type);
        insert.addBindValue(finished);
        insert.addBindValue(libraryType);

        if (!insert.exec() || !m_db.commit()) {
            rLog() << "Library" << "Failed to insert show to DB:" << insert.lastError().text();
            m_db.rollback();
            return false;
        }
        if (affectsDisplay) {
            LibraryEntry e;
            e.link             = show.link;
            e.title            = show.title;
            e.cover            = show.coverUrl;
            e.provider         = show.provider ? show.provider->name() : QString();
            e.libraryType      = libraryType;
            e.lastWatchedIndex = lastWatchedIndex;
            e.finished         = (finished != 0);
            e.timestamp        = timestamp;
            e.totalEpisodes    = totalEpisodes;
            e.showType         = show.type;
            e.valid            = true;
            int insertIndex = m_displayCache.size();
            beginInsertRows(QModelIndex(), insertIndex, insertIndex);
            m_displayCache.push_back(e);
            endInsertRows();
        }
        // Search-added shows have no playlist yet - fetch the count so the badge appears now.
        if (totalEpisodes <= 0)
            fetchUnwatchedEpisodes(libraryType, true);
        emit libraryChanged();
    }

    return true;
}

bool LibraryManager::linkExists(const QString &link) const {
    if (link.isEmpty()) return false;
    QSqlQuery query(m_db);
    query.prepare("SELECT 1 FROM shows WHERE link = ? LIMIT 1");
    query.addBindValue(link);
    return query.exec() && query.next();
}

QString LibraryManager::linkAtIndex(int index, int libraryType) const {
    if (libraryType == m_displayLibraryType)
        return (index >= 0 && index < m_displayCache.size()) ? m_displayCache[index].link : QString();
    QSqlQuery query(m_db);
    query.prepare("SELECT link FROM shows WHERE library_type = ? ORDER BY sort_order, link LIMIT 1 OFFSET ?");
    query.addBindValue(libraryType);
    query.addBindValue(index);
    if (query.exec() && query.next())
        return query.value(0).toString();
    return QString();
}

int LibraryManager::count(int libraryType) const {
    if (libraryType == -1) libraryType = m_displayLibraryType;
    if (libraryType == m_displayLibraryType)
        return m_displayCache.size();
    QSqlQuery query(m_db);
    query.prepare("SELECT COUNT(*) FROM shows WHERE library_type = ?");
    query.addBindValue(libraryType);
    if (!query.exec() || !query.next())
        return 0;
    return query.value(0).toInt();
}

void LibraryManager::remove(const QString &link) {
    int previousLibraryType = getLibraryType(link);
    int prevIndex = (previousLibraryType == m_displayLibraryType) ? indexOf(link) : -1;

    QSqlQuery query(m_db);
    m_db.transaction();
    query.prepare("DELETE FROM shows WHERE link = ?");
    query.addBindValue(link);
    if (!query.exec() || !m_db.commit()) {
        m_db.rollback();
        return;
    }

    if (prevIndex >= 0) {
        beginRemoveRows(QModelIndex(), prevIndex, prevIndex);
        m_displayCache.removeAt(prevIndex);
        endRemoveRows();
    }
    emit libraryChanged();
}

void LibraryManager::removeAt(int index, int libraryType) {
    if (libraryType == -1) libraryType = m_displayLibraryType;
    QString link = linkAtIndex(index, libraryType);
    if (!link.isEmpty())
        remove(link);
}

void LibraryManager::move(int from, int to) {
    if (from == to || from < 0 || to < 0) return;
    if (from >= m_displayCache.size() || to >= m_displayCache.size()) return;

    // Reorder inside the begin/endMoveRows bracket (Qt contract).
    beginMoveRows(QModelIndex(), from, from, QModelIndex(), to + (to > from ? 1 : 0));
    m_displayCache.move(from, to);
    endMoveRows();

    // Persist the new ordering as a contiguous sort_order for this library_type.
    m_db.transaction();
    QSqlQuery update(m_db);
    update.prepare("UPDATE shows SET sort_order = ? WHERE link = ?");
    for (int i = 0; i < m_displayCache.size(); ++i) {
        update.addBindValue(i);
        update.addBindValue(m_displayCache[i].link);
        if (!update.exec()) {
            rLog() << "Library" << "Failed to persist move:" << update.lastError().text();
            m_db.rollback();
            return;
        }
    }
    if (!m_db.commit()) {
        rLog() << "Library" << "Failed to commit move";
        m_db.rollback();
    }
}

void LibraryManager::updateProgress(const QString &link, int lastWatchedIndex, int timestamp, bool completed) {
    QSqlQuery query(m_db);
    query.prepare("UPDATE shows SET last_watched_index = ?, timestamp = ?, finished = ? WHERE link = ?");
    query.addBindValue(lastWatchedIndex);
    query.addBindValue(timestamp);
    query.addBindValue(completed ? 1 : 0);
    query.addBindValue(link);
    if (!query.exec()) return;
    int idx = indexOf(link);
    if (idx >= 0) {
        auto &e = m_displayCache[idx];
        e.lastWatchedIndex = lastWatchedIndex;
        e.timestamp = timestamp;
        e.finished = completed;
        emit dataChanged(index(idx), index(idx));
    }
}

void LibraryManager::cacheHistoryMeta(const QString &link, const QString &title,
                                      const QString &cover, const QString &provider, int total) {
    if (link.isEmpty()) return;
    m_historyMeta[link] = { title, cover, provider, total };
}

void LibraryManager::recordHistory(const QString &link, int lastWatchedIndex, int timestamp) {
    // Only record shows loaded this session (their metadata is cached). A cleared cache means the
    // outgoing show's final progress save won't resurrect it right after the user clears history.
    if (!m_historyMeta.contains(link)) return;
    const HistoryMeta meta = m_historyMeta.value(link);
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO history "
              "(link, title, cover, provider, last_watched_index, timestamp, total_episodes, last_played_at) "
              "VALUES (?, ?, ?, ?, ?, ?, ?, strftime('%s','now')) "
              "ON CONFLICT(link) DO UPDATE SET "
              "  title = CASE WHEN excluded.title != '' THEN excluded.title ELSE title END,"
              "  cover = CASE WHEN excluded.cover != '' THEN excluded.cover ELSE cover END,"
              "  provider = CASE WHEN excluded.provider != '' THEN excluded.provider ELSE provider END,"
              "  total_episodes = CASE WHEN excluded.total_episodes > 0 THEN excluded.total_episodes ELSE total_episodes END,"
              "  last_watched_index = excluded.last_watched_index,"
              "  timestamp = excluded.timestamp,"
              "  last_played_at = excluded.last_played_at");
    q.addBindValue(link);
    q.addBindValue(meta.title);
    q.addBindValue(meta.cover);
    q.addBindValue(meta.provider);
    q.addBindValue(lastWatchedIndex);
    q.addBindValue(timestamp);
    q.addBindValue(meta.total);
    if (q.exec()) emit historyChanged();
}

void LibraryManager::clearHistory() {
    QSqlQuery q(m_db);
    if (q.exec("DELETE FROM history")) emit historyChanged();
}

LibraryManager::HistoryEntry LibraryManager::getHistoryEntry(const QString &link) const {
    HistoryEntry e;
    QSqlQuery q(m_db);
    q.prepare("SELECT title, cover, provider, last_watched_index, timestamp, total_episodes "
              "FROM history WHERE link = ?");
    q.addBindValue(link);
    if (q.exec() && q.next()) {
        e.link = link;
        e.title = q.value(0).toString();
        e.cover = q.value(1).toString();
        e.provider = q.value(2).toString();
        e.lastWatchedIndex = q.value(3).toInt();
        e.timestamp = q.value(4).toInt();
        e.totalEpisodes = q.value(5).toInt();
        e.valid = true;
    }
    return e;
}

void LibraryManager::updateShowCover(const QString &link, const QString &cover) {
    QSqlQuery query(m_db);
    query.prepare("UPDATE shows SET cover = ? WHERE link = ?");
    query.addBindValue(cover);
    query.addBindValue(link);
    if (!query.exec()) {
        rLog() << "Library" << "Failed to update show cover";
        return;
    }
    int idx = indexOf(link);
    if (idx >= 0) {
        m_displayCache[idx].cover = cover;
        emit dataChanged(index(idx), index(idx));
    }
}

ShowData::LastWatchInfo LibraryManager::getLastWatchInfo(const QString &showLink) {
    ShowData::LastWatchInfo info;
    QSqlQuery query(m_db);
    query.prepare("SELECT library_type, last_watched_index, timestamp FROM shows WHERE link = ?");
    query.addBindValue(showLink);
    if (query.exec() && query.next()) {
        info.libraryType = query.value(0).toInt();
        info.lastWatchedIndex = query.value(1).toInt();
        info.timestamp = query.value(2).toInt();
    }
    return info;
}

LibraryManager::LibraryEntry LibraryManager::getEntry(int index) const {
    return (index >= 0 && index < m_displayCache.size()) ? m_displayCache[index] : LibraryEntry{};
}

void LibraryManager::changeLibraryTypeAt(int index, int newLibraryType, int oldLibraryType) {
    if (oldLibraryType == -1) oldLibraryType = m_displayLibraryType;
    QString link = linkAtIndex(index, oldLibraryType);
    changeLibraryType(link, newLibraryType);
}

void LibraryManager::changeLibraryType(const QString &link, int libraryType) {
    if (!linkExists(link)) return;

    int oldLibraryType = getLibraryType(link);
    if (oldLibraryType == libraryType) return;

    int oldIndex = (oldLibraryType == m_displayLibraryType) ? indexOf(link) : -1;

    m_db.transaction();
    int nextSortOrder = 0;
    {
        QSqlQuery q(m_db);
        q.prepare("SELECT IFNULL(MAX(sort_order), -1) + 1 FROM shows WHERE library_type = ?");
        q.addBindValue(libraryType);
        if (!q.exec() || !q.next()) {
            rLog() << "Library" << "Failed to get next sort_order:" << q.lastError().text();
            m_db.rollback();
            return;
        }
        nextSortOrder = q.value(0).toInt();
    }

    QSqlQuery update(m_db);
    update.prepare("UPDATE shows SET library_type = ?, sort_order = ? WHERE link = ?");
    update.addBindValue(libraryType);
    update.addBindValue(nextSortOrder);
    update.addBindValue(link);

    if (!update.exec() || !m_db.commit()) {
        rLog() << "Library" << "Failed to update libraryType:" << update.lastError().text();
        m_db.rollback();
        return;
    }

    if (oldIndex >= 0) {
        beginRemoveRows(QModelIndex(), oldIndex, oldIndex);
        m_displayCache.removeAt(oldIndex);
        endRemoveRows();
    }
    if (libraryType == m_displayLibraryType) {
        LibraryEntry e = entryForLink(link);
        if (e.valid) {
            int insertIndex = m_displayCache.size();
            beginInsertRows(QModelIndex(), insertIndex, insertIndex);
            m_displayCache.push_back(e);
            endInsertRows();
            if (e.totalEpisodes <= 0)
                fetchUnwatchedEpisodes(libraryType, true);   // populate the badge for the new arrival
        }
    }
    emit libraryChanged();
}

int LibraryManager::getLibraryType(const QString &link) const {
    QSqlQuery query(m_db);
    query.prepare("SELECT library_type FROM shows WHERE link = ?");
    query.addBindValue(link);
    if (query.exec() && query.next())
        return query.value(0).toInt();
    return -1;
}

void LibraryManager::fetchUnwatchedEpisodes(int libraryType, bool force) {
    if (libraryType < 0 || libraryType > LibraryType::COMPLETED) return;

    if (!force) {
        const qint64 last = m_lastFetchMs.value(libraryType, 0);
        if (last != 0 && QDateTime::currentMSecsSinceEpoch() - last < kFetchDebounceMs)
            return;   // refreshed recently - skip the network round-trip
    }

    if (m_fetchWatcher.isRunning()) {
        // Queue the request; the finished() handler will restart it.
        m_cancel.cancel();
        m_pendingFetchLibraryType = libraryType;
        m_pendingFetchForced = force;
        return;
    }
    m_pendingFetchLibraryType = k_noPendingFetch;
    m_cancel.reset();

    QSqlQuery query(m_db);
    query.prepare("SELECT link, provider FROM shows WHERE library_type = ?");
    query.addBindValue(libraryType);
    if (!query.exec()) return;
    QList<QPair<QString, ShowProvider*>> shows;
    while (query.next()) {
        QString providerName = query.value(1).toString();
        ShowProvider *provider = ProviderManager::getProvider(providerName);
        if (!provider) continue;
        shows.emplaceBack(query.value(0).toString(), provider);
    }

    m_fetchWatcher.setFuture(QtConcurrent::run([this, shows, libraryType] {
        // Bounded batches so a large library doesn't fire one job per show at once.
        constexpr int batchSize = 6;
        QList<QPair<QString,int>> results;
        results.reserve(shows.size());

        for (int start = 0; start < shows.size() && !m_cancel.isCancelled(); start += batchSize) {
            const int end = std::min(static_cast<int>(shows.size()), start + batchSize);
            QList<QFuture<QPair<QString, int>>> jobs;
            jobs.reserve(end - start);
            for (int i = start; i < end; ++i) {
                const auto show = shows[i];
                jobs.push_back(QtConcurrent::run([this, show]() mutable {
                    auto client = Client(m_cancel, false);
                    auto dummyShow = ShowData("", show.first);
                    int totalEpisodes = 0;
                    try {
                        totalEpisodes = show.second->getEpisodeCount(&client, dummyShow);
                    } catch (AppException &e) {
                        e.print();
                    } catch (const std::exception &e) {
                        oLog() << "Library" << show.first << e.what();
                    } catch (...) {
                        oLog() << "Library" << show.first << "unknown error";
                    }
                    return QPair<QString, int>(show.first, totalEpisodes);
                }));
            }
            for (auto &job : jobs) {
                job.waitForFinished();
                auto result = job.result();
                if (!result.first.isEmpty()) results.append(result);
            }
        }

        QMetaObject::invokeMethod(this, [this, results, libraryType]() {
            m_lastFetchMs[libraryType] = QDateTime::currentMSecsSinceEpoch();

            // Drop count-0 results so a transient provider error doesn't wipe a known count.
            QList<QPair<QString, int>> valid;
            valid.reserve(results.size());
            for (const auto &p : std::as_const(results))
                if (p.second > 0) valid.append(p);

            if (valid.isEmpty()) {
                emit fetchedAllEpCounts();
                return;
            }

            QVariantList totals;
            QVariantList links;
            totals.reserve(valid.size());
            links.reserve(valid.size());

            for (const auto &p : std::as_const(valid)) {
                links.append(p.first);
                totals.append(p.second);
            }

            QSqlQuery updateQuery(m_db);
            if (!m_db.transaction()) {
                rLog() << "Library" << "Failed to start transaction for batch update:" << m_db.lastError().text();
            }

            updateQuery.prepare("UPDATE shows SET total_episodes = ? WHERE link = ?");
            updateQuery.addBindValue(totals);
            updateQuery.addBindValue(links);

            if (!updateQuery.execBatch()) {
                rLog() << "Library" << "execBatch failed, falling back to individual updates:" << updateQuery.lastError().text();
                m_db.rollback();

                if (!m_db.transaction()) {
                    rLog() << "Library" << "Failed to start transaction for fallback updates:" << m_db.lastError().text();
                } else {
                    QSqlQuery single(m_db);
                    single.prepare("UPDATE shows SET total_episodes = ? WHERE link = ?");
                    for (const auto &p : std::as_const(valid)) {
                        single.bindValue(0, p.second);
                        single.bindValue(1, p.first);
                        if (!single.exec()) {
                            rLog() << "Library" << "Failed to update total_episodes for" << p.first << ":" << single.lastError().text();
                        }
                    }
                    if (!m_db.commit()) {
                        rLog() << "Library" << "Failed to commit fallback transaction:" << m_db.lastError().text();
                        m_db.rollback();
                    }
                }
            } else {
                if (!m_db.commit()) {
                    rLog() << "Library" << "Failed to commit batch update:" << m_db.lastError().text();
                    m_db.rollback();
                }
            }

            for (const auto &p : std::as_const(valid)) {
                int idx = indexOf(p.first);
                if (idx >= 0 && m_displayCache[idx].totalEpisodes != p.second) {
                    m_displayCache[idx].totalEpisodes = p.second;
                    emit dataChanged(index(idx), index(idx));
                }
            }

            emit fetchedAllEpCounts();
        }, Qt::QueuedConnection);
    }));
}

void LibraryManager::setDisplayLibraryType(int newLibraryType) {
    if (m_displayLibraryType != newLibraryType) {
        beginResetModel();
        m_displayLibraryType = newLibraryType;
        refreshDisplayCache();
        endResetModel();
        emit libraryTypeChanged();
        fetchUnwatchedEpisodes(newLibraryType);   // refresh counts for the newly shown tab
    }
}
