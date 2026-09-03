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

void LibraryProxyModel::setSortMode(int mode) {
    if (m_sortMode == mode) return;
    m_sortMode = mode;
    // sort() early-returns on an unchanged column, so drop it first to force a re-sort.
    sort(-1);
    if (mode != Manual) sort(0, Qt::AscendingOrder);
    emit sortModeChanged();
}

bool LibraryProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const {
    switch (m_sortMode) {
    case TitleAscending:
        return left.data(Library::Role::Title).toString().compare(
                   right.data(Library::Role::Title).toString(), Qt::CaseInsensitive) < 0;
    // Return "greater" as "less" so the most unwatched sorts first.
    case MostUnwatched:
        return left.data(Library::Role::UnwatchedEpisodes).toInt() > right.data(Library::Role::UnwatchedEpisodes).toInt();
    default:
        return QSortFilterProxyModel::lessThan(left, right);
    }
}

bool LibraryProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const {
    if (m_titleFilter.isEmpty() && m_typeFilter == 0 && !m_hasUnwatchedEpisodesOnly)
        return true;

    const auto idx = sourceModel()->index(sourceRow, 0, sourceParent);

    if (m_typeFilter != 0 && idx.data(Library::Role::ShowType).toInt() != m_typeFilter)
        return false;

    // Hide only shows known fully watched (== 0); unknown (-1) stays visible.
    if (m_hasUnwatchedEpisodesOnly && idx.data(Library::Role::UnwatchedEpisodes).toInt() == 0)
        return false;

    if (!m_titleFilter.isEmpty()) {
        QString title = idx.data(Library::Role::Title).toString();
        if (m_useRegex)
            return m_titleRegex.isValid() && m_titleRegex.match(title).hasMatch();
        else
            return title.contains(m_titleFilter, m_caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);
    }

    return true;
}
