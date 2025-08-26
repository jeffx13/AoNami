#pragma once

#include <QAbstractListModel>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonObject>
#include <QJsonArray>
#include <QtConcurrent/QtConcurrentRun>
#include "servicemanager.h"
#include "showdata.h"

class LibraryManager : public ServiceManager
{
    Q_OBJECT
    Q_PROPERTY(int libraryType READ getDisplayLibraryType WRITE setDisplayLibraryType NOTIFY modelReset)

public:
    // --- Types ---
    enum LibraryType {
        WATCHING,
        PLANNED,
        PAUSED,
        DROPPED,
        COMPLETED
    };

    // --- Constructors/Destructors ---
    explicit LibraryManager(QObject *parent = nullptr);
    ~LibraryManager();

    // --- Core API ---
    Q_INVOKABLE int count(int libraryType = -1) const;
    Q_INVOKABLE bool add(const ShowData& show, int libraryType);
    Q_INVOKABLE void removeAt(int index, int libraryType = -1);
    Q_INVOKABLE void remove(const QString &link);
    Q_INVOKABLE void move(int from, int to);

    // --- Library Type Management ---
    Q_INVOKABLE void cycleDisplayLibraryType() { setDisplayLibraryType((m_displayLibraryType + 1) % 5); }
    Q_INVOKABLE void changeLibraryTypeAt(int index, int newLibraryType, int oldLibraryType = -1);
    void changeLibraryType(const QString &link, int newLibraryType);
    Q_INVOKABLE int getLibraryType(const QString &link) const;
    int getDisplayLibraryType() const { return m_displayLibraryType; }
    void setDisplayLibraryType(int newLibraryType);

    // --- Data/Progress ---
    QVariant getData(int index, const QString &key);
    Q_INVOKABLE void fetchUnwatchedEpisodes(int libraryType);
    ShowData::LastWatchInfo getLastWatchInfo(const QString& showLink);
    Q_INVOKABLE void updateProgress(const QString &link, int lastWatchedIndex, int timestamp);
    void updateShowCover(const QString &link, const QString &cover);

signals:
    void aboutToInsert(int startIndex, int count);
    void inserted();
    void aboutToMove(int fromIndex, int toIndex);
    void moved();
    void aboutToRemove(int index);
    void removed();
    void modelReset();
    void changed(int index);
    void fetchedAllEpCounts();

private:
    // --- Internal helpers ---
    void initDatabase();
    int indexOf(const QString &link);
    bool linkExists(const QString &link) const;
    QString linkAtIndex(int index, int libraryType) const;

    // --- Member variables ---
    QSqlDatabase m_db;
    int m_displayLibraryType = WATCHING;
    QFuture<void> m_fetchUnwatchedEpisodesJob;
    std::atomic<bool> m_isCancelled = false;
};

