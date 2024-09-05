#pragma once

#include <QSortFilterProxyModel>

class LibraryProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(QString titleFilter READ titleFilter WRITE setTitleFilter NOTIFY titleFilterChanged FINAL)
    Q_PROPERTY(int typeFilter READ typeFilter WRITE setTypeFilter NOTIFY typeFilterChanged FINAL)
public:
    explicit LibraryProxyModel(QObject *parent = nullptr) : QSortFilterProxyModel(parent) {
        setDynamicSortFilter(true);
    };
    QString titleFilter() const;
    void setTitleFilter(const QString &newTitleFilter);

    bool useRegex() const;
    void setUseRegex(bool newUseRegex);

    bool caseSensitive() const;
    void setCaseSensitive(bool newCaseSensitive);

    int typeFilter() const;
    void setTypeFilter(int newTypeFilter);

signals:
    void typeFilterChanged();
    void titleFilterChanged();
    void useRegexChanged();
    void caseSensitiveChanged();
protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

private:
    QString m_titleFilter;
    bool m_useRegex = false;
    bool m_caseSensitive = false;
    int m_typeFilter = 0;
};

