#include "libraryproxymodel.h"



QString LibraryProxyModel::titleFilter() const {
    return m_titleFilter;
}

void LibraryProxyModel::setTitleFilter(const QString &newTitleFilter) {
    if (m_titleFilter == newTitleFilter)
        return;
    beginFilterChange();
    m_titleFilter = newTitleFilter;
    emit titleFilterChanged();
    endFilterChange();
}

bool LibraryProxyModel::useRegex() const
{
    return m_useRegex;
}

void LibraryProxyModel::setUseRegex(bool newUseRegex)
{
    if (m_useRegex == newUseRegex)
        return;
    beginFilterChange();
    m_useRegex = newUseRegex;
    m_titleRegex = QRegularExpression(m_titleFilter, m_caseSensitive ? QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption);
    emit useRegexChanged();
    endFilterChange();
}

bool LibraryProxyModel::caseSensitive() const
{
    return m_caseSensitive;
}

void LibraryProxyModel::setCaseSensitive(bool caseSensitive)
{
    if (m_caseSensitive == caseSensitive)
        return;
    beginFilterChange();
    m_caseSensitive = caseSensitive;
    emit caseSensitiveChanged();
    endFilterChange();
}

int LibraryProxyModel::typeFilter() const
{
    return m_typeFilter;
}

void LibraryProxyModel::setTypeFilter(int newTypeFilter)
{
    if (m_typeFilter == newTypeFilter)
        return;
    beginFilterChange();
    m_typeFilter = newTypeFilter;
    emit typeFilterChanged();
    endFilterChange();
}

bool LibraryProxyModel::hasUnwatchedEpisodesOnly() const { return m_hasUnwatchedEpisodesOnly; }

void LibraryProxyModel::setHasUnwatchedEpisodesOnly(bool hasUnwatchedEpisodesOnly) {
    if (m_hasUnwatchedEpisodesOnly == hasUnwatchedEpisodesOnly) return;
    beginFilterChange();
    m_hasUnwatchedEpisodesOnly = hasUnwatchedEpisodesOnly;
    emit hasUnwatchedEpisodesOnlyChanged();
    endFilterChange();
}

bool LibraryProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const {
    if (m_titleFilter.isEmpty() && m_typeFilter == 0 && !m_hasUnwatchedEpisodesOnly) return true;

    const auto index = sourceModel()->index(sourceRow, 0, sourceParent);
    auto title = index.data(TitleRole).toString();
    auto showType = m_typeFilter == 0 ? m_typeFilter : index.data(TypeRole).toInt();
    auto hasUnwatchedEpisodes = m_hasUnwatchedEpisodesOnly ? index.data(UnwatchedEpisodesRole).toInt() > 0 : false;

    bool typeAccepted = m_typeFilter == 0 || showType == m_typeFilter;
    bool titleAccepted = m_titleFilter.isEmpty() ? true : m_useRegex ? m_titleRegex.match(title).hasMatch() : title.contains(m_titleFilter, m_caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);


    return titleAccepted && typeAccepted && hasUnwatchedEpisodes;
}
