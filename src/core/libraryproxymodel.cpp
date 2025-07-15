#include "libraryproxymodel.h"



QString LibraryProxyModel::titleFilter() const {
    return m_titleFilter;
}

void LibraryProxyModel::setTitleFilter(const QString &newTitleFilter) {
    if (m_titleFilter == newTitleFilter)
        return;
    m_titleFilter = newTitleFilter;
    emit titleFilterChanged();
    invalidateFilter();
}

bool LibraryProxyModel::useRegex() const
{
    return m_useRegex;
}

void LibraryProxyModel::setUseRegex(bool newUseRegex)
{
    if (m_useRegex == newUseRegex)
        return;

    m_useRegex = newUseRegex;
    m_titleRegex = QRegularExpression(m_titleFilter, m_caseSensitive ? QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption);

    emit useRegexChanged();
    invalidateFilter();
}

bool LibraryProxyModel::caseSensitive() const
{
    return m_caseSensitive;
}

void LibraryProxyModel::setCaseSensitive(bool newCaseSensitive)
{
    if (m_caseSensitive == newCaseSensitive)
        return;
    m_caseSensitive = newCaseSensitive;
    emit caseSensitiveChanged();
    invalidateFilter();
}

int LibraryProxyModel::typeFilter() const
{
    return m_typeFilter;
}

void LibraryProxyModel::setTypeFilter(int newTypeFilter)
{
    if (m_typeFilter == newTypeFilter)
        return;

    m_typeFilter = newTypeFilter;
    emit typeFilterChanged();
    invalidateFilter();
}

bool LibraryProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const {
    if (m_titleFilter.isEmpty() && m_typeFilter == 0) return true;

    const auto index = sourceModel()->index(sourceRow, 0, sourceParent);
    auto title = index.data(Qt::UserRole).toString();
    auto showType = m_typeFilter == 0 ? m_typeFilter : index.data(Qt::UserRole + 3).toInt();

    bool typeAccepted = m_typeFilter == 0 || showType == m_typeFilter;
    bool titleAccepted = m_titleFilter.isEmpty() ? true : m_useRegex ? m_titleRegex.match(title).hasMatch() : title.contains(m_titleFilter, m_caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);

    return titleAccepted && typeAccepted;
}
