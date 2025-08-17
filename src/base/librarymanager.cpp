#include "librarymanager.h"
#include "providermanager.h"
#include "providers/showprovider.h"
#include "base/network/network.h"
#include <QDir>
#include <QCoreApplication>
#include <QSqlRecord>

LibraryManager::LibraryManager(QObject *parent)
    : ServiceManager(parent)
{
    initDatabase();
}

LibraryManager::~LibraryManager() {
    m_isCancelled = true;
    m_fetchUnwatchedEpisodesJob.waitForFinished();
}

static bool migrateLegacyJsonToSql(QSqlDatabase db, const QString& legacyPath)
{
    QFile f(legacyPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Cannot open legacy file:" << legacyPath;
        return false;
    }
    const QByteArray bytes = f.readAll();
    f.close();

    QJsonParseError perr;
    const QJsonDocument doc = QJsonDocument::fromJson(bytes, &perr);
    if (perr.error != QJsonParseError::NoError || !doc.isArray()) {
        qWarning() << "Legacy JSON parse error:" << perr.errorString();
        return false;
    }

    const QJsonArray outer = doc.array();            // [ [ {...}, ... ], [ {...}, ... ], ... ]
    if (outer.isEmpty()) return true;

    if (!db.transaction()) {
        qWarning() << "Failed to start migration transaction:" << db.lastError().text();
        return false;
    }

    // We'll batch insert per library list for clarity and speed.
    for (int libType = 0; libType < outer.size(); ++libType) {
        if (!outer.at(libType).isArray()) continue;
        const QJsonArray arr = outer.at(libType).toArray();
        if (arr.isEmpty()) continue;

        // Collect batch values
        QVariantList v_link, v_title, v_provider, v_library_type,
            v_last_watched_index, v_timestamp, v_cover,
            v_total_episodes, v_sort_order;

        v_link.reserve(arr.size());
        v_title.reserve(arr.size());
        v_provider.reserve(arr.size());
        v_library_type.reserve(arr.size());
        v_last_watched_index.reserve(arr.size());
        v_timestamp.reserve(arr.size());
        v_cover.reserve(arr.size());
        v_total_episodes.reserve(arr.size());
        v_sort_order.reserve(arr.size());

        for (int i = 0; i < arr.size(); ++i) {
            if (!arr.at(i).isObject()) {
                qDebug() << arr.at(i);
            }
            const QJsonObject o = arr.at(i).toObject();

            QString link   = o.value("link").toString();


            if (link.isEmpty()) {
                link = QString::number(libType) + "&" + QString::number(i);
                qDebug() << "invalid" << link;
            }

            const QString title  = o.value("title").toString();
            const QString provider = o.value("provider").toString();
            const QString cover  = o.value("cover").toString();

            // Keys are camelCase in legacy JSON:
            const int lastWatchedIndex = o.value("lastWatchedIndex").toInt(-1);
            const int timeStamp        = o.value("timeStamp").toInt(0);

            QSqlQuery ins(db);
            ins.prepare(R"(INSERT INTO shows
    (link, title, provider, cover, library_type, last_watched_index, timestamp, total_episodes, sort_order)
    VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?))");

            ins.addBindValue(link);
            ins.addBindValue(title);
            ins.addBindValue(provider);
            ins.addBindValue(cover);
            ins.addBindValue(libType);
            ins.addBindValue(lastWatchedIndex);
            ins.addBindValue(timeStamp);
            ins.addBindValue(0);
            ins.addBindValue(i);
            if (!ins.exec()) {
                qWarning() << "Insert failed (library_type" << libType << "index" << i
                           << "):" << ins.lastError().text();
                db.rollback();
                return false;
            }
        }




    }

    if (!db.commit()) {
        qWarning() << "Migration commit failed:" << db.lastError().text();
        db.rollback();
        return false;
    }

    qInfo() << "Legacy library migration completed.";
    return true;
}

void LibraryManager::initDatabase() {
    QString dbPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + QDir::separator() + "library.db");
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qCritical() << "Failed to open SQLite DB:" << m_db.lastError().text();
        return;
    }

    QSqlQuery query(m_db);
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
            sort_order INTEGER
        )
    )")) {
        rLog() << "Library" << "Failed to create table:" << query.lastError().text();
        return;
    }
}

int LibraryManager::indexOf(const QString &link) {
    if (link.isEmpty()) return -1;
    QSqlQuery query;
    query.prepare(R"(
        SELECT COUNT(*)
        FROM shows
        WHERE library_type = :library_type
          AND sort_order < (SELECT sort_order
                            FROM shows
                            WHERE library_type = :library_type
                              AND link = :link
                            LIMIT 1))");
    query.bindValue(":library_type", m_displayLibraryType);
    query.bindValue(":link", link);
    if (!query.exec()) {
        rLog() << "Library" << "indexOf failed:" << query.lastError().text();
        return -1;
    }

    if (query.next()) {
        return query.value(0).toInt();
    }

    return -1;

}

bool LibraryManager::add(ShowData& show, int libraryType) {
    // Check if show already exists
    QSqlQuery check;
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
        // Insert new
        QSqlQuery insert;
        m_db.transaction();
        insert.prepare(R"(
            INSERT INTO shows (link, title, provider, cover, library_type, last_watched_index, timestamp, total_episodes, sort_order)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, (
                SELECT IFNULL(MAX(sort_order), -1) + 1 FROM shows WHERE library_type = ?
            ))
        )");
        auto playlist = show.getPlaylist();
        auto lastWatchedIndex = playlist ? playlist->getCurrentIndex() : -1;
        auto totalEpisodes = playlist ? playlist->count() : 0;
        auto timestamp = lastWatchedIndex == -1 ? 0 : playlist->getCurrentItem()->getTimestamp();
        insert.addBindValue(show.link);
        insert.addBindValue(show.title);
        insert.addBindValue(show.provider->name());
        insert.addBindValue(show.coverUrl);
        insert.addBindValue(libraryType);
        insert.addBindValue(lastWatchedIndex);
        insert.addBindValue(timestamp);
        insert.addBindValue(totalEpisodes);
        insert.addBindValue(libraryType);

        if (libraryType == m_displayLibraryType) {
            emit aboutToInsert(count(libraryType), 1);
        }
        if (!insert.exec() || !m_db.commit()) {
            rLog() << "Library" << "Failed to insert and commit show to DB:" << insert.lastError().text();
            m_db.rollback();
            emit inserted();
            emit modelReset();
            return false;
        }
        if (libraryType == m_displayLibraryType) {
            emit inserted();
        }
    }



    return true;
}

bool LibraryManager::linkExists(const QString &link) const {
    if (link.isEmpty()) return false;
    QSqlQuery query;
    query.prepare("SELECT 1 FROM shows WHERE link = ? LIMIT 1");
    query.addBindValue(link);
    return query.exec() && query.next();
}

QString LibraryManager::linkAtIndex(int index, int libraryType) const {
    QSqlQuery query;
    query.prepare("SELECT link FROM shows WHERE library_type = ? ORDER BY sort_order, link LIMIT 1 OFFSET ?");
    query.addBindValue(libraryType);
    query.addBindValue(index);
    if (query.exec() && query.next())
        return query.value(0).toString();
    return QString();
}

int LibraryManager::count(int libraryType) const {
    if (libraryType == -1) libraryType = m_displayLibraryType;
    QSqlQuery query;
    query.prepare("SELECT COUNT(*) FROM shows WHERE library_type = ?");
    query.addBindValue(libraryType);
    if (!query.exec() || !query.next())
        return 0;
    return query.value(0).toInt();
}



void LibraryManager::removeByLink(const QString &link) {
    QSqlQuery query;
    m_db.transaction();
    query.prepare("DELETE FROM shows WHERE link = ?");
    query.addBindValue(link);
    if (getLibraryType(link) == m_displayLibraryType) {
        emit aboutToRemove(indexOf(link));
    }
    if (!query.exec() || !m_db.commit()) {
        m_db.rollback();
        emit removed();
        emit modelReset();
        return;
    }

    if (getLibraryType(link) == m_displayLibraryType) {
        emit moved();
    }
}

void LibraryManager::removeAt(int index, int libraryType) {
    if (libraryType == -1) libraryType = m_displayLibraryType;
    QString link = linkAtIndex(index, libraryType);
    if (!link.isEmpty())
        removeByLink(link);
}

void LibraryManager::move(int from, int to) {
    if (from == to || from < 0 || to < 0) return;
    QString movingLink = linkAtIndex(from, m_displayLibraryType);
    if (movingLink.isEmpty()) return;

    m_db.transaction();

    // Determine current sort_orders
    QSqlQuery query(m_db);
    query.prepare("SELECT link, sort_order FROM shows WHERE library_type = ? ORDER BY sort_order");
    query.addBindValue(m_displayLibraryType);
    if (!query.exec()) {
        rLog() << "Library" << "Failed to get list for move:" << query.lastError().text();
        m_db.rollback();
        return;
    }

    QList<QPair<QString, int>> list;
    while (query.next()) {
        list.append({ query.value(0).toString(), query.value(1).toInt() });
    }

    if (from >= list.size() || to >= list.size()) {
        m_db.rollback();
        return;
    }

    // Move the item in the list
    auto item = list.takeAt(from);
    list.insert(to, item);

    // Reassign sort_order
    emit aboutToMove(from, to);
    bool success = true;
    for (int i = 0; i < list.size(); ++i) {
        if (list[i].second != i) { // only update if changed
            QSqlQuery update(m_db);
            update.prepare("UPDATE shows SET sort_order = ? WHERE link = ?");
            update.addBindValue(i);
            update.addBindValue(list[i].first);
            if (!update.exec()) {
                rLog() << "Library" << "Failed to update sort_order:" << update.lastError().text();
                m_db.rollback();
                success = false;
                break;
            }
        }
    }

    emit moved();
    if (!success || !m_db.commit()) {
        rLog() << "Library" << "Failed to commit move";
        m_db.rollback();
        emit modelReset();
    }
}

void LibraryManager::updateProgress(const QString &link, int lastWatchedIndex, int timestamp) {
    QSqlQuery query(m_db);
    m_db.transaction();
    query.prepare("UPDATE shows SET last_watched_index = ?, timestamp = ? WHERE link = ?");
    query.addBindValue(lastWatchedIndex);
    query.addBindValue(timestamp);
    query.addBindValue(link);
    if (!query.exec() || !m_db.commit()) {
        m_db.rollback();
        return;
    }

    emit changed(indexOf(link));
}

void LibraryManager::updateShowCover(const QString &link, const QString &cover) {
    QSqlQuery query;
    m_db.transaction();
    query.prepare("UPDATE shows SET cover = ? WHERE link = ?");
    query.addBindValue(cover);
    query.addBindValue(link);
    if (!query.exec() || !m_db.commit()) {
        rLog() << "Library" << "Failed to update show cover";
        m_db.rollback();
        return;
    }
    emit changed(indexOf(link));
}

ShowData::LastWatchInfo LibraryManager::getLastWatchInfo(const QString &showLink) {
    ShowData::LastWatchInfo info;
    QSqlQuery query;
    query.prepare("SELECT library_type, last_watched_index, timestamp FROM shows WHERE link = ?");
    query.addBindValue(showLink);
    if (query.exec() && query.next()) {
        info.libraryType = query.value(0).toInt();
        info.lastWatchedIndex = query.value(1).toInt();
        info.timestamp = query.value(2).toInt();
    }
    return info;
}

QVariant LibraryManager::getData(int index, const QString &key) {
    static const QSet<QString> allowedKeys {"link", "title", "cover", "provider", "library_type", "sort_order", "last_watched_index", "timestamp", "total_episodes"};
    auto keyList = key.split(',', Qt::SkipEmptyParts);
    if (key != "*") {
        Q_FOREACH(const auto &k, keyList) {
            if (!allowedKeys.contains(k.trimmed()))
                return QVariant();
        }
    }
    QString columnList = (key == "*") ? "*" : keyList.join(", ");
    QString sql = QString("SELECT %1 FROM shows WHERE library_type = ? ORDER BY sort_order LIMIT 1 OFFSET ?")
                      .arg(columnList);

    QSqlQuery query;
    query.prepare(sql);
    query.addBindValue(m_displayLibraryType);
    query.addBindValue(index);


    if (!query.exec() || !query.next())
        return QVariant();

    if (keyList.size() > 1 || key == "*") {
        QVariantMap rowData;
        QSqlRecord rec = query.record();
        for (int i = 0; i < rec.count(); ++i)
            rowData.insert(rec.fieldName(i), query.value(i));
        return rowData;
    } else {
        return query.value(0);
    }
}

void LibraryManager::changeLibraryTypeAt(int index, int newLibraryType, int oldLibraryType) {
    if (oldLibraryType == -1) oldLibraryType = m_displayLibraryType;
    QString link = linkAtIndex(index, oldLibraryType);
    changeLibraryType(link, newLibraryType);
}

void LibraryManager::changeLibraryType(const QString &link, int libraryType) {
    if (!linkExists(link)) return;
    // Move to new libraryType at bottom
    m_db.transaction();
    int nextSortOrder = 0;
    {
        QSqlQuery q;
        q.prepare("SELECT IFNULL(MAX(sort_order), -1) + 1 FROM shows WHERE library_type = ?");
        q.addBindValue(libraryType);
        if (!q.exec() || !q.next()) {
            rLog() << "Library" << "Failed to get next sort_order:" << q.lastError().text();
            m_db.rollback();
            return;
        }
        nextSortOrder = q.value(0).toInt();
    }

    QSqlQuery update;
    update.prepare(R"(
            UPDATE shows
            SET library_type = ?, sort_order = ?
            WHERE link = ?
        )");
    update.addBindValue(libraryType);
    update.addBindValue(nextSortOrder);
    update.addBindValue(link);
    if (!update.exec() || !m_db.commit()) {
        rLog() << "Library" << "Failed to update libraryType:" << update.lastError().text();
        m_db.rollback();
        return;
    }
    emit modelReset();
}

int LibraryManager::getLibraryType(const QString &link) const {
    QSqlQuery query;
    query.prepare("SELECT library_type FROM shows WHERE link = ?");
    query.addBindValue(link);
    if (query.exec() && query.next())
        return query.value(0).toInt();
    return -1;
}





void LibraryManager::fetchUnwatchedEpisodes(int libraryType) {
    if (m_fetchUnwatchedEpisodesJob.isRunning()) {
        m_isCancelled = true;
        m_fetchUnwatchedEpisodesJob.waitForFinished();
    }
    if (libraryType < 0 || libraryType > 4) return;
    QSqlQuery query;
    query.prepare("SELECT link, provider FROM shows WHERE library_type = ?");
    query.addBindValue(libraryType);
    if (!query.exec()) return;
    QList<QPair<QString, ShowProvider*>> shows;
    while (query.next()) {
        if (m_isCancelled) return;
        QString providerName = query.value(1).toString();
        ShowProvider *provider = ProviderManager::getProvider(providerName);
        if (!provider) continue;
        QString link = query.value(0).toString();
        shows.emplaceBack(link, provider);
    }

    m_isCancelled = false;
    m_fetchUnwatchedEpisodesJob = QtConcurrent::run([this, shows] {
        QList<QFuture<QPair<QString, int>>> jobs;
        Q_FOREACH(const auto &show, shows) {
            QFuture<QPair<QString, int>> job  = QtConcurrent::run([this, show]() mutable {
                auto client = Client(&m_isCancelled, false);
                auto dummyShow = ShowData("", show.first);
                int totalEpisodes = 0;
                try {
                    totalEpisodes = show.second->getEpisodeCount(&client, dummyShow);
                } catch (MyException &e) {
                    e.print();
                }
                return QPair<QString, int>(show.first, totalEpisodes);
            });
            jobs.push_back(job);
        }
        QList<QPair<QString,int>> results; // populate this from each finished job, same as you already did

        for (auto &job: jobs) {
            job.waitForFinished();
            auto result = job.result();
            if (result.first.isEmpty()) continue;
            results.append(result);
        }

        QMetaObject::invokeMethod(this, [this, results]() {
            if (results.isEmpty()) {
                emit fetchedAllEpCounts();
                return;
            }

            // Prepare lists for batch binding
            QVariantList totals;
            QVariantList links;
            totals.reserve(results.size());
            links.reserve(results.size());

            for (const auto &p : results) {
                links.append(p.first);
                totals.append(p.second);
            }

            // execBatch (much faster)
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

                // Fallback: do individual updates in a transaction
                if (!m_db.transaction()) {
                    rLog() << "Library" << "Failed to start transaction for fallback updates:" << m_db.lastError().text();
                } else {
                    QSqlQuery single(m_db);
                    single.prepare("UPDATE shows SET total_episodes = ? WHERE link = ?");
                    for (const auto &p : results) {
                        single.bindValue(0, p.second);
                        single.bindValue(1, p.first);
                        if (!single.exec()) {
                            rLog() << "Library" << "Failed to update total_episodes for" << p.first << ":" << single.lastError().text();
                            // continue — we still try to update the rest; consider whether to rollback and abort
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

            emit fetchedAllEpCounts();
        }, Qt::QueuedConnection);


    });
}

void LibraryManager::setDisplayLibraryType(int newLibraryType) {
    if (m_displayLibraryType != newLibraryType) {
        m_displayLibraryType = newLibraryType;
        emit modelReset();
    }
}



