#pragma once
#include <QFutureWatcher>
#include <qqmlintegration.h>
#include "app/listmodel.h"
#include "providers/subdl.h"

class MpvPlayer;

// Deliberately separate from the player's track list; a result reaches mpv only once picked.
class SubtitleSearch : public ListModel
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(bool    isLoading READ isLoading NOTIFY isLoadingChanged)
    Q_PROPERTY(int     count     READ count     NOTIFY countChanged)
    Q_PROPERTY(QString query     READ query     NOTIFY queryChanged)
public:
    enum { DisplayNameRole = Qt::UserRole, ReleaseRole, LanguageRole, AuthorRole,
           EpisodeRole, HiRole, TagsRole, SlotRole, FetchingRole };

    explicit SubtitleSearch(QObject *parent = nullptr);
    ~SubtitleSearch();

    Q_INVOKABLE void search(const QString &query);
    // Skips a query that already finished, so re-opening the page costs nothing.
    Q_INVOKABLE void searchIfNew(const QString &query);
    Q_INVOKABLE void use(int row, bool secondary = false);
    Q_INVOKABLE void cancel();

    int  count() const { return m_rows.size(); }
    bool isLoading() const { return m_searchWatcher.isRunning(); }
    QString query() const { return m_query; }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_SIGNAL void isLoadingChanged();
    Q_SIGNAL void countChanged();
    Q_SIGNAL void queryChanged();

private:
    // Everything data() returns is precomputed: it runs per row per repaint.
    struct Row {
        SubDl::Result result;
        QString       displayName;
        QStringList   tags;
        QString       localPath;      // set once downloaded
        int           slot = 0;       // 0 none, 1 primary, 2 secondary
        bool          fetching = false;
    };

    void setResults(const QList<SubDl::Result> &results);
    void refreshSlots();
    int  rowForFileId(const QString &fileId) const;
    // MpvPlayer is built by QML, so it does not exist yet when this is constructed.
    MpvPlayer *mpv();

    QList<Row>                           m_rows;
    QString                              m_query;
    QString                              m_searchedQuery;   // last query that finished
    QFutureWatcher<QList<SubDl::Result>> m_searchWatcher;
    QFutureWatcher<void>                 m_fetchWatcher;
    bool                                 m_mpvConnected = false;
};
