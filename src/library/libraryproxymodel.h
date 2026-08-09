#pragma once
#include <QSortFilterProxyModel>
#include <QRegularExpression>
#include "library/librarymanager.h"  // For LibraryRoles
#include <qqmlintegration.h>

class LibraryProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(QString titleFilter              READ titleFilter              WRITE setTitleFilter              NOTIFY titleFilterChanged)
    Q_PROPERTY(int     typeFilter               READ typeFilter               WRITE setTypeFilter               NOTIFY typeFilterChanged)
    Q_PROPERTY(bool    hasUnwatchedEpisodesOnly READ hasUnwatchedEpisodesOnly WRITE setHasUnwatchedEpisodesOnly NOTIFY hasUnwatchedEpisodesOnlyChanged)
    Q_PROPERTY(bool    caseSensitive            READ caseSensitive            WRITE setCaseSensitive            NOTIFY caseSensitiveChanged)
    Q_PROPERTY(bool    useRegex                 READ useRegex                 WRITE setUseRegex                 NOTIFY useRegexChanged)
    Q_PROPERTY(int     sortRole                 READ sortRole                 WRITE setSortRole                 NOTIFY sortRoleChanged)

public:
    explicit LibraryProxyModel(QObject *parent = nullptr)
        : QSortFilterProxyModel(parent) { setDynamicSortFilter(true); }

    QString titleFilter() const { return m_titleFilter; }
    void setTitleFilter(const QString &filter);

    bool useRegex() const { return m_useRegex; }
    void setUseRegex(bool enabled);

    bool caseSensitive() const { return m_caseSensitive; }
    void setCaseSensitive(bool sensitive);

    int typeFilter() const { return m_typeFilter; }
    void setTypeFilter(int type);

    bool hasUnwatchedEpisodesOnly() const { return m_hasUnwatchedEpisodesOnly; }
    void setHasUnwatchedEpisodesOnly(bool enabled);

    int  sortRole() const { return m_sortRole; }
    void setSortRole(int role);

    Q_INVOKABLE int mapToAbsoluteIndex(int proxyIndex) const {
        return mapToSource(index(proxyIndex, 0)).row();
    }

    // Replaces deprecated QSortFilterProxyModel::invalidate()
    Q_INVOKABLE void refreshFilter() {
        beginFilterChange();
        endFilterChange();
    }

signals:
    void titleFilterChanged();
    void typeFilterChanged();
    void useRegexChanged();
    void caseSensitiveChanged();
    void hasUnwatchedEpisodesOnlyChanged();
    void sortRoleChanged();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    void rebuildRegex();

    QString m_titleFilter;
    QRegularExpression m_titleRegex;
    bool m_useRegex                 = false;
    bool m_caseSensitive            = false;
    bool m_hasUnwatchedEpisodesOnly = true;
    int  m_typeFilter               = 0;
    int  m_sortRole                 = 0;
};
