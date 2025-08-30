#pragma once
#include <QAbstractListModel>
#include "forwards.h"

class SearchResultModel: public QAbstractListModel
{
    Q_OBJECT
public:
    SearchResultModel(const SearchResultModel &) = delete;
    SearchResultModel(SearchResultModel &&) = delete;
    SearchResultModel &operator=(const SearchResultModel &) = delete;
    SearchResultModel &operator=(SearchResultModel &&) = delete;
    explicit SearchResultModel(SearchManager *searchManager, QObject *parent = nullptr);
    enum {
        TitleRole = Qt::UserRole,
        CoverRole,
        LinkRole
    };

    Q_INVOKABLE void reset();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
private:
    SearchManager *m_searchManager;
};
