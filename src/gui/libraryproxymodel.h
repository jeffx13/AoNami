#pragma once

#include <QSortFilterProxyModel>

class LibraryProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(QString titleFilter READ titleFilter WRITE setTitleFilter NOTIFY titleFilterChanged FINAL)
    Q_PROPERTY(int typeFilter READ typeFilter WRITE setTypeFilter NOTIFY typeFilterChanged FINAL)
    Q_PROPERTY(bool hasUnwatchedEpisodesOnly READ hasUnwatchedEpisodesOnly WRITE setHasUnwatchedEpisodesOnly NOTIFY hasUnwatchedEpisodesOnlyChanged FINAL)
public:
    explicit LibraryProxyModel(QObject *parent = nullptr) : QSortFilterProxyModel(parent) {
        setDynamicSortFilter(true);
    };
    QString titleFilter() const;
    void setTitleFilter(const QString &newTitleFilter);

    bool useRegex() const;
    void setUseRegex(bool newUseRegex);

    bool caseSensitive() const;
    void setCaseSensitive(bool caseSensitive);

    int typeFilter() const;
    void setTypeFilter(int newTypeFilter);

    bool hasUnwatchedEpisodesOnly() const;
    void setHasUnwatchedEpisodesOnly(bool hasUnwatchedEpisodesOnly);

    Q_INVOKABLE int mapToAbsoluteIndex(int proxyIndex) const {
        return mapToSource(index(proxyIndex, 0)).row();
    }
signals:
    void typeFilterChanged();
    void titleFilterChanged();
    void useRegexChanged();
    void caseSensitiveChanged();
    void hasUnwatchedEpisodesOnlyChanged();
protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

private:
    QString m_titleFilter;
    QRegularExpression m_titleRegex;
    bool m_useRegex                 = false;
    bool m_caseSensitive            = false;
    bool m_hasUnwatchedEpisodesOnly = true;
    int m_typeFilter = 0;
    enum {
        TitleRole = Qt::UserRole,
        CoverRole,
        UnwatchedEpisodesRole,
        TypeRole,
    };
};

