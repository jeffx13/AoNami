#include "library/librarymanager.h"
#include "providers/providermanager.h"
#include "providers/showprovider.h"
#include "net/client.h"
#include "ui/uibridge.h"
#include "app/async.h"
#include "app/logger.h"
#include "app/settings.h"
#include <QDir>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QVariant>
#include <QDateTime>
#include <algorithm>

LibraryManager::LibraryManager(QObject *parent)
    : QAbstractListModel(parent)
{
    initDatabase();
    refreshDisplayCache();
    m_watchedFraction = Settings::instance().watchedFraction();
    connect(&Settings::instance(), &Settings::watchedPercentChanged, this, [this]() {
        m_watchedFraction = Settings::instance().watchedFraction();
        if (!m_displayCache.isEmpty())
            emit dataChanged(index(0), index(m_displayCache.size() - 1));
        emit historyChanged();
    });
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
        int unwatched = e.totalEpisodes - e.lastWatchedIndex - 1;
        if (e.lastWatchedIndex >= 0 && e.progress < m_watchedFraction)
            unwatched += 1;
        return qMax(0, unwatched);
    }
    case TypeRole: return e.showType;
    case ProviderRole: return e.provider;
    default: return {};
    }
}

QHash<int, QByteArray> LibraryManager::roleNames() const {
    using namespace LibraryRoles;
    return {
        {TitleRole, "title"}, {CoverRole, "cover"},
        {UnwatchedEpisodesRole, "unwatchedEpisodes"}, {TypeRole, "type"},
        {ProviderRole, "provider"},
    };
}

LibraryManager::~LibraryManager() {
    disconnect(&m_fetchWatcher, nullptr, this, nullptr);
    m_cancel.cancel();
    waitFor(m_fetchWatcher, "LibraryManager episode-count fetch");
    m_db.close();
    m_db = QSqlDatabase();
    QSqlDatabase::removeDatabase(QStringLiteral("AoNami_Library"));
}

void LibraryManager::initDatabase() {
    QString dbPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/library.db");
    m_db = QSqlDatabase::addDatabase("QSQLITE", QStringLiteral("AoNami_Library"));
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        const QString err = m_db.lastError().text();
        rLog() << "Library" << "Failed to open SQLite DB:" << err;
        // Queued: the notifier doesn't exist yet during construction.
        QMetaObject::invokeMethod(qApp, [err]() {
            UiBridge::instance().showError("Library database could not be opened:\n" + err +
                                           "\nYour library and history will not be saved this session.",
                                           "Library");
        }, Qt::QueuedConnection);
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
            total_episodes INTEGER,
            sort_order INTEGER,
            show_type INTEGER DEFAULT 0,
            progress REAL DEFAULT 0
        )
    )")) {
        rLog() << "Library" << "Failed to create table:" << query.lastError().text();
        return;
    }

    query.exec("ALTER TABLE shows ADD COLUMN show_type INTEGER DEFAULT 0");

    auto hasColumn = [this](const QString &table, const QString &column) {
        QSqlQuery cols(m_db);
        if (!cols.exec("PRAGMA table_info(" + table + ")")) return true;
        while (cols.next())
            if (cols.value(1).toString() == column) return true;
        return false;
    };

    if (!hasColumn("shows", "progress"))
        query.exec("ALTER TABLE shows ADD COLUMN progress REAL DEFAULT 0");

    // Watch history is independent of the library so shows you never add still get tracked.
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS history (
            link TEXT PRIMARY KEY,
            title TEXT,
            cover TEXT,
            provider TEXT,
            last_watched_index INTEGER,
            total_episodes INTEGER,
            last_played_at INTEGER DEFAULT 0,
            progress REAL DEFAULT 0
        )
    )");

    if (!hasColumn("history", "progress"))
        query.exec("ALTER TABLE history ADD COLUMN progress REAL DEFAULT 0");

    // Still holding the column is the once-only guard; seconds cannot convert to a fraction.
    for (const QString &table : {QStringLiteral("shows"), QStringLiteral("history")}) {
        if (!hasColumn(table, "timestamp")) continue;
        if (!query.exec("ALTER TABLE " + table + " DROP COLUMN timestamp"))
            rLog() << "Library" << "Could not drop" << table << "timestamp:" << query.lastError().text();
        query.exec("UPDATE " + table + " SET progress = 0");
    }

    // Superseded by last_watched_index long ago; nothing has read or written it since.
    if (hasColumn("shows", "watched_index"))
        query.exec("ALTER TABLE shows DROP COLUMN watched_index");

    // finished stored progress against whatever the threshold was that day; derived, it follows it.
    for (const QString &table : {QStringLiteral("shows"), QStringLiteral("history")})
        if (hasColumn(table, "finished"))
            query.exec("ALTER TABLE " + table + " DROP COLUMN finished");

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
    entry.totalEpisodes    = query.value(6).toInt();
    entry.showType         = query.value(7).toInt();
    entry.progress         = query.value(8).toDouble();
    entry.valid            = true;
    return entry;
}

void LibraryManager::refreshDisplayCache() {
    m_displayCache.clear();
    QSqlQuery query(m_db);
    query.prepare("SELECT link, title, cover, provider, library_type, last_watched_index, total_episodes, show_type, progress "
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
    query.prepare("SELECT link, title, cover, provider, library_type, last_watched_index, total_episodes, show_type, progress "
                  "FROM shows WHERE link = ?");
    query.addBindValue(link);
    if (query.exec() && query.next())
        return entryFromQuery(query);
    return {};
}

QVariantMap LibraryManager::entryAt(int index) const {
    QVariantMap m;
    if (index < 0 || index >= m_displayCache.size()) return m;
    const LibraryEntry &e = m_displayCache[index];
    m["link"]             = e.link;
    m["title"]            = e.title;
    m["cover"]            = e.cover;
    m["provider"]         = e.provider;
    m["libraryType"]      = e.libraryType;
    m["lastWatchedIndex"] = e.lastWatchedIndex;
    m["progress"]         = e.progress;
    m["totalEpisodes"]    = e.totalEpisodes;
    return m;
}

bool LibraryManager::migrate(const QString &oldLink, const QString &newLink, const QString &title,
                             const QString &cover, const QString &provider, int showType,
                             int lastWatchedIndex, int totalEpisodes) {
    if (oldLink.isEmpty() || newLink.isEmpty()) return false;
    if (newLink != oldLink && linkExists(newLink)) return false;

    if (!m_db.transaction()) return false;
    QSqlQuery q(m_db);
    q.prepare("UPDATE shows SET link=?, title=?, cover=?, provider=?, show_type=?, "
              "last_watched_index=?, total_episodes=? WHERE link=?");
    q.addBindValue(newLink);      q.addBindValue(title);   q.addBindValue(cover);
    q.addBindValue(provider);     q.addBindValue(showType);
    q.addBindValue(lastWatchedIndex); q.addBindValue(totalEpisodes);
    q.addBindValue(oldLink);
    if (!q.exec() || q.numRowsAffected() == 0) { m_db.rollback(); return false; }

    QSqlQuery h(m_db);
    h.prepare("DELETE FROM history WHERE link = ?");   h.addBindValue(newLink);
    if (!h.exec()) { m_db.rollback(); return false; }
    h.prepare("UPDATE history SET link=?, title=?, cover=?, provider=?, last_watched_index=?, "
              "total_episodes=? WHERE link=?");
    h.addBindValue(newLink); h.addBindValue(title); h.addBindValue(cover); h.addBindValue(provider);
    h.addBindValue(lastWatchedIndex); h.addBindValue(totalEpisodes);
    h.addBindValue(oldLink);
    if (!h.exec()) { m_db.rollback(); return false; }
    if (!m_db.commit()) { m_db.rollback(); return false; }

    m_historyMeta.remove(oldLink);
    beginResetModel();
    refreshDisplayCache();
    endResetModel();
    emit libraryChanged();
    emit historyChanged();
    return true;
}

QVariantList LibraryManager::history() const {
    QVariantList list;
    QSqlQuery query(m_db);
    query.prepare("SELECT link, title, cover, last_watched_index, total_episodes, progress "
                  "FROM history ORDER BY last_played_at DESC LIMIT 100");
    if (!query.exec()) return list;
    while (query.next()) {
        const int lwi   = query.value(3).toInt();
        const int total = query.value(4).toInt();
        // Counting the in-progress episode whole read one episode ahead.
        const double raw = qBound(0.0, query.value(5).toDouble(), 1.0);
        const double watched = raw >= m_watchedFraction ? 1.0 : raw;
        QVariantMap m;
        m["link"]     = query.value(0).toString();
        m["title"]    = query.value(1).toString();
        m["cover"]    = query.value(2).toString();
        m["episode"]  = lwi >= 0 ? lwi + 1 : 0;
        m["total"]    = total;
        m["progress"] = (total > 0 && lwi >= 0) ? qBound(0.0, (lwi + watched) / total, 1.0) : 0.0;
        m["episodeProgress"] = lwi >= 0 ? watched : 0.0;
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
        auto progress = currentItem ? currentItem->getProgress() : 0.0;

        bool affectsDisplay = (libraryType == m_displayLibraryType);

        QSqlQuery insert(m_db);
        m_db.transaction();
        insert.prepare(R"(
            INSERT INTO shows (link, title, provider, cover, library_type, last_watched_index, progress, total_episodes, show_type, sort_order)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, (
                SELECT IFNULL(MAX(sort_order), -1) + 1 FROM shows WHERE library_type = ?
            ))
        )");
        insert.addBindValue(show.link);
        insert.addBindValue(show.title);
        insert.addBindValue(show.provider ? show.provider->name() : QString());
        insert.addBindValue(show.coverUrl);
        insert.addBindValue(libraryType);
        insert.addBindValue(lastWatchedIndex);
        insert.addBindValue(progress);
        insert.addBindValue(totalEpisodes);
        insert.addBindValue(show.type);
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
            e.progress         = progress;
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
    QSqlQuery query(m_db);
    m_db.transaction();
    query.prepare("DELETE FROM shows WHERE link = ?");
    query.addBindValue(link);
    if (!query.exec() || !m_db.commit()) {
        m_db.rollback();
        return;
    }

    // Trust the cache, not the row's claimed type: they disagree after a type change without a refresh.
    if (const int row = indexOf(link); row >= 0) {
        beginRemoveRows(QModelIndex(), row, row);
        m_displayCache.removeAt(row);
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

    // Persist first: a failed write used to leave the cache reordered and the DB untouched.
    QList<LibraryEntry> reordered = m_displayCache;
    reordered.move(from, to);

    if (!m_db.transaction()) return;
    QSqlQuery update(m_db);
    update.prepare("UPDATE shows SET sort_order = ? WHERE link = ?");
    for (int i = 0; i < reordered.size(); ++i) {
        update.addBindValue(i);
        update.addBindValue(reordered[i].link);
        if (!update.exec()) {
            rLog() << "Library" << "Failed to persist move:" << update.lastError().text();
            m_db.rollback();
            return;
        }
    }
    if (!m_db.commit()) {
        rLog() << "Library" << "Failed to commit move";
        m_db.rollback();
        return;
    }

    beginMoveRows(QModelIndex(), from, from, QModelIndex(), to + (to > from ? 1 : 0));
    m_displayCache.move(from, to);
    endMoveRows();
}

void LibraryManager::updateProgress(const QString &link, int lastWatchedIndex, double progress) {
    QSqlQuery query(m_db);
    query.prepare("UPDATE shows SET last_watched_index = ?, progress = ? WHERE link = ?");
    query.addBindValue(lastWatchedIndex);
    query.addBindValue(progress);
    query.addBindValue(link);
    query.exec();

    // last_played_at stays put: this is progress within a play, not a new one.
    QSqlQuery hq(m_db);
    hq.prepare("UPDATE history SET last_watched_index = ?, progress = ? WHERE link = ?");
    hq.addBindValue(lastWatchedIndex);
    hq.addBindValue(progress);
    hq.addBindValue(link);
    if (hq.exec() && hq.numRowsAffected() > 0) emit historyChanged();

    int idx = indexOf(link);
    if (idx >= 0) {
        auto &e = m_displayCache[idx];
        e.lastWatchedIndex = lastWatchedIndex;
        e.progress = progress;
        emit dataChanged(index(idx), index(idx));
    }
}

void LibraryManager::cacheHistoryMeta(const QString &link, const QString &title,
                                      const QString &cover, const QString &provider, int total) {
    if (link.isEmpty()) return;
    m_historyMeta[link] = { title, cover, provider, total };
}

void LibraryManager::recordHistory(const QString &link, int lastWatchedIndex) {
    // Only shows loaded this session, so clearing history isn't undone by the outgoing show's last save.
    if (!m_historyMeta.contains(link)) return;
    const HistoryMeta meta = m_historyMeta.value(link);
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO history "
              "(link, title, cover, provider, last_watched_index, total_episodes, last_played_at) "
              "VALUES (?, ?, ?, ?, ?, ?, strftime('%s','now')) "
              "ON CONFLICT(link) DO UPDATE SET "
              "  title = CASE WHEN excluded.title != '' THEN excluded.title ELSE title END,"
              "  cover = CASE WHEN excluded.cover != '' THEN excluded.cover ELSE cover END,"
              "  provider = CASE WHEN excluded.provider != '' THEN excluded.provider ELSE provider END,"
              "  total_episodes = CASE WHEN excluded.total_episodes > 0 THEN excluded.total_episodes ELSE total_episodes END,"
              "  last_watched_index = excluded.last_watched_index,"
              "  last_played_at = excluded.last_played_at");
    q.addBindValue(link);
    q.addBindValue(meta.title);
    q.addBindValue(meta.cover);
    q.addBindValue(meta.provider);
    q.addBindValue(lastWatchedIndex);
    q.addBindValue(meta.total);
    if (q.exec()) emit historyChanged();
}

void LibraryManager::clearHistory() {
    QSqlQuery q(m_db);
    if (!q.exec("DELETE FROM history")) return;
    m_historyMeta.clear();   // otherwise the next progress save re-inserts what was just cleared
    emit historyChanged();
}

void LibraryManager::removeFromHistory(const QString &link) {
    if (link.isEmpty()) return;
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM history WHERE link = ?");
    q.addBindValue(link);
    if (!q.exec()) return;
    m_historyMeta.remove(link);
    emit historyChanged();
}

LibraryManager::HistoryEntry LibraryManager::getHistoryEntry(const QString &link) const {
    HistoryEntry e;
    QSqlQuery q(m_db);
    q.prepare("SELECT title, cover, provider, last_watched_index, total_episodes, progress "
              "FROM history WHERE link = ?");
    q.addBindValue(link);
    if (q.exec() && q.next()) {
        e.link = link;
        e.title = q.value(0).toString();
        e.cover = q.value(1).toString();
        e.provider = q.value(2).toString();
        e.lastWatchedIndex = q.value(3).toInt();
        e.totalEpisodes = q.value(4).toInt();
        e.progress = q.value(5).toDouble();
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
    query.prepare("SELECT library_type, last_watched_index, progress FROM shows WHERE link = ?");
    query.addBindValue(showLink);
    if (query.exec() && query.next()) {
        info.libraryType = query.value(0).toInt();
        info.lastWatchedIndex = query.value(1).toInt();
        info.progress = query.value(2).toDouble();
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

    const int oldIndex = indexOf(link);

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
                fetchUnwatchedEpisodes(libraryType, true);
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

        QMetaObject::invokeMethod(this, [this, results, libraryType, partial = m_cancel.isCancelled()]() {
            // Stamping a cancelled run would debounce away the refetch that replaced it.
            if (!partial) m_lastFetchMs[libraryType] = QDateTime::currentMSecsSinceEpoch();

            // Drop count-0 results so a transient provider error doesn't wipe a known count.
            QList<QPair<QString, int>> valid;
            valid.reserve(results.size());
            for (const auto &p : std::as_const(results))
                if (p.second > 0) valid.append(p);

            if (valid.isEmpty()) {
                emit fetchedAllEpCounts();
                return;
            }

            persistEpisodeCounts(valid);

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

void LibraryManager::persistEpisodeCounts(const QList<QPair<QString, int>> &counts) {
    static const QLatin1String sql("UPDATE shows SET total_episodes = ? WHERE link = ?");
    if (!m_db.transaction()) {
        rLog() << "Library" << "Could not open a transaction for episode counts:" << m_db.lastError().text();
        return;
    }

    QVariantList totals, links;
    totals.reserve(counts.size());
    links.reserve(counts.size());
    for (const auto &[link, total] : counts) {
        links.append(link);
        totals.append(total);
    }

    QSqlQuery batch(m_db);
    batch.prepare(sql);
    batch.addBindValue(totals);
    batch.addBindValue(links);

    // Some drivers refuse execBatch; the same rows one at a time reach the identical end state.
    if (!batch.execBatch()) {
        rLog() << "Library" << "execBatch failed, updating one row at a time:" << batch.lastError().text();
        QSqlQuery single(m_db);
        single.prepare(sql);
        for (const auto &[link, total] : counts) {
            single.bindValue(0, total);
            single.bindValue(1, link);
            if (!single.exec())
                rLog() << "Library" << "Could not set total_episodes for" << link << ":" << single.lastError().text();
        }
    }

    if (!m_db.commit()) {
        rLog() << "Library" << "Could not commit episode counts:" << m_db.lastError().text();
        m_db.rollback();
    }
}

void LibraryManager::setDisplayLibraryType(int newLibraryType) {
    // Written through a QML property, and an out-of-range one would just show an empty library.
    newLibraryType = qBound<int>(WATCHING, newLibraryType, COMPLETED);
    if (m_displayLibraryType != newLibraryType) {
        beginResetModel();
        m_displayLibraryType = newLibraryType;
        refreshDisplayCache();
        endResetModel();
        emit libraryTypeChanged();
        fetchUnwatchedEpisodes(newLibraryType);
    }
}
