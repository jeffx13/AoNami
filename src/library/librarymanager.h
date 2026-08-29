#pragma once
#include <QAbstractListModel>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QtConcurrent/QtConcurrentRun>
#include <QFutureWatcher>
#include <QVariantList>
#include <QHash>
#include "app/listmodel.h"
#include "providers/showdata.h"
#include <qqmlintegration.h>

namespace LibraryRoles {
enum Role { TitleRole = Qt::UserRole, CoverRole, UnwatchedEpisodesRole, TypeRole, ProviderRole };
}

class LibraryManager : public ListModel
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(int libraryType READ getDisplayLibraryType WRITE setDisplayLibraryType NOTIFY libraryTypeChanged)
public:
    enum LibraryType { WATCHING, PLANNED, PAUSED, DROPPED, COMPLETED };

    explicit LibraryManager(QObject *parent = nullptr);
    ~LibraryManager();

    Q_INVOKABLE int  count(int libraryType = -1) const;
    Q_INVOKABLE int  getLibraryType(const QString &link) const;
    Q_INVOKABLE bool linkExists(const QString &link) const;
    ShowData::LastWatchInfo getLastWatchInfo(const QString &showLink);

    struct LibraryEntry {
        QString title;
        QString link;
        QString cover;
        QString provider;
        int libraryType = -1;
        int lastWatchedIndex = -1;   // the episode last watched (resume point + badge base)
        double progress = 0.0;       // how far into that episode, 0..1
        int totalEpisodes = 0;
        int showType = 0;       // ShowData::ShowType (ANIME, MOVIE, etc.)
        bool valid = false;
    };
    LibraryEntry getEntry(int index) const;
    LibraryEntry getEntryByLink(const QString &link) const { return entryForLink(link); }

    // roleNames() omits link, so QML needs this to read a whole row.
    Q_INVOKABLE QVariantMap entryAt(int index) const;

    bool migrate(const QString &oldLink, const QString &newLink, const QString &title,
                 const QString &cover, const QString &provider, int showType,
                 int lastWatchedIndex, int totalEpisodes);

    Q_INVOKABLE QVariantList history() const;
    Q_INVOKABLE void clearHistory();
    Q_INVOKABLE void removeFromHistory(const QString &link);

    // Stamped on episode start, not on position saves, or clearing history undoes itself.
    void recordHistory(const QString &link, int lastWatchedIndex);

    // Cover and title only exist in ShowData, so cache them at load - shows outside the library get tracked too.
    void cacheHistoryMeta(const QString &link, const QString &title, const QString &cover,
                          const QString &provider, int total);

    struct HistoryEntry {
        QString link, title, cover, provider;
        int lastWatchedIndex = -1, totalEpisodes = 0;
        double progress = 0.0;
        bool valid = false;
    };
    HistoryEntry getHistoryEntry(const QString &link) const;

    Q_INVOKABLE bool add(const ShowData &show, int libraryType);
    Q_INVOKABLE void removeAt(int index, int libraryType = -1);
    Q_INVOKABLE void remove(const QString &link);
    Q_INVOKABLE void move(int from, int to);
    Q_INVOKABLE void changeLibraryTypeAt(int index, int newLibraryType, int oldLibraryType = -1);
    void changeLibraryType(const QString &link, int newLibraryType);
    Q_INVOKABLE void cycleDisplayLibraryType() { setDisplayLibraryType((m_displayLibraryType + 1) % (COMPLETED + 1)); }

    Q_INVOKABLE void updateProgress(const QString &link, int lastWatchedIndex, double progress);
    void updateShowCover(const QString &link, const QString &cover);

    Q_INVOKABLE void fetchUnwatchedEpisodes(int libraryType, bool force = false);

    int getDisplayLibraryType() const { return m_displayLibraryType; }
    void setDisplayLibraryType(int newLibraryType);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

signals:
    void fetchedAllEpCounts();
    void libraryTypeChanged();
    void libraryChanged();   // any membership change (add/remove/type) - for badges outside the current view
    void historyChanged();   // a play was recorded or history was cleared

private:
    void initDatabase();
    int indexOf(const QString &link);
    QString linkAtIndex(int index, int libraryType) const;

    // In-memory snapshot of the displayed library_type; the model reads rows from here, not SQL.
    void refreshDisplayCache();
    LibraryEntry entryForLink(const QString &link) const;
    static LibraryEntry entryFromQuery(const QSqlQuery &query);
    QList<LibraryEntry> m_displayCache;

    static constexpr int k_noPendingFetch = -2;
    static constexpr qint64 kFetchDebounceMs = 60'000;   // skip re-fetch within this window

    QSqlDatabase m_db;
    int m_displayLibraryType = WATCHING;
    QFutureWatcher<void> m_fetchWatcher;
    int  m_pendingFetchLibraryType = k_noPendingFetch;
    bool m_pendingFetchForced = false;
    QHash<int, qint64> m_lastFetchMs;   // library type -> epoch ms of last completed fetch

    struct HistoryMeta { QString title, cover, provider; int total = 0; };
    QHash<QString, HistoryMeta> m_historyMeta;   // link -> display metadata, populated at show load
    double m_watchedFraction = 0.8;              // mirrors the setting; re-read when it changes
};
