#include "library/libraryproxymodel.h"

void LibraryProxyModel::rebuildRegex() {
    if (!m_useRegex) return;
    m_titleRegex = QRegularExpression(
        m_titleFilter,
        m_caseSensitive ? QRegularExpression::NoPatternOption
                        : QRegularExpression::CaseInsensitiveOption);
}

void LibraryProxyModel::setTitleFilter(const QString &filter) {
    if (m_titleFilter == filter) return;
    beginFilterChange();
    m_titleFilter = filter;
    rebuildRegex();
    emit titleFilterChanged();
    endFilterChange();
}

void LibraryProxyModel::setUseRegex(bool enabled) {
    if (m_useRegex == enabled) return;
    beginFilterChange();
    m_useRegex = enabled;
    rebuildRegex();
    emit useRegexChanged();
    endFilterChange();
}

void LibraryProxyModel::setCaseSensitive(bool sensitive) {
    if (m_caseSensitive == sensitive) return;
    beginFilterChange();
    m_caseSensitive = sensitive;
    rebuildRegex();
    emit caseSensitiveChanged();
    endFilterChange();
}

void LibraryProxyModel::setTypeFilter(int type) {
    if (m_typeFilter == type) return;
    beginFilterChange();
    m_typeFilter = type;
    emit typeFilterChanged();
    endFilterChange();
}

void LibraryProxyModel::setHasUnwatchedEpisodesOnly(bool enabled) {
    if (m_hasUnwatchedEpisodesOnly == enabled) return;
    beginFilterChange();
    m_hasUnwatchedEpisodesOnly = enabled;
    emit hasUnwatchedEpisodesOnlyChanged();
    endFilterChange();
}

void LibraryProxyModel::setSortRole(int role) {
    if (m_sortRole == role) return;
    m_sortRole = role;
    if (role == 0)
        sort(-1);
    else
        sort(0, Qt::AscendingOrder);
    emit sortRoleChanged();
}

bool LibraryProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const {
    using namespace LibraryRoles;
    switch (m_sortRole) {
    case 1: // A-Z
        return left.data(TitleRole).toString().compare(
                   right.data(TitleRole).toString(), Qt::CaseInsensitive) < 0;
    case 2: // Most unwatched first (return "greater" value as "less" to sort descending)
        return left.data(UnwatchedEpisodesRole).toInt() > right.data(UnwatchedEpisodesRole).toInt();
    default:
        return QSortFilterProxyModel::lessThan(left, right);
    }
}

bool LibraryProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const {
    // Fast path: no filters active
    if (m_titleFilter.isEmpty() && m_typeFilter == 0 && !m_hasUnwatchedEpisodesOnly)
        return true;

    const auto idx = sourceModel()->index(sourceRow, 0, sourceParent);
    using namespace LibraryRoles;

    if (m_typeFilter != 0 && idx.data(TypeRole).toInt() != m_typeFilter)
        return false;

    // Hide only shows known fully watched (== 0); unknown (-1) stays visible.
    if (m_hasUnwatchedEpisodesOnly && idx.data(UnwatchedEpisodesRole).toInt() == 0)
        return false;

    if (!m_titleFilter.isEmpty()) {
        QString title = idx.data(TitleRole).toString();
        if (m_useRegex)
            return m_titleRegex.isValid() && m_titleRegex.match(title).hasMatch();
        else
            return title.contains(m_titleFilter, m_caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);
    }

    return true;
}
