#pragma once
#include <QAbstractListModel>
#include "base/searchmanager.h"
#include "base/showdata.h"


class SearchResultModel: public QAbstractListModel
{
    Q_OBJECT
public:
    SearchResultModel(const SearchResultModel &) = delete;
    SearchResultModel(SearchResultModel &&) = delete;
    SearchResultModel &operator=(const SearchResultModel &) = delete;
    SearchResultModel &operator=(SearchResultModel &&) = delete;
    SearchResultModel(SearchManager *searchManager, QObject *parent = nullptr)
        : m_searchManager(searchManager), QAbstractListModel(parent) {
        connect(m_searchManager, &SearchManager::aboutToInsert, this,
                [this](int startIndex, int endIndex) {
            beginInsertRows(QModelIndex(), startIndex, endIndex);
        });
        connect(m_searchManager, &SearchManager::inserted, this, &SearchResultModel::endInsertRows);
        connect(m_searchManager, &SearchManager::modelReset, this, &SearchResultModel::reset);
    }
    enum {
        TitleRole = Qt::UserRole,
        CoverRole,
        LinkRole
    };

    Q_INVOKABLE void reset() {
        beginResetModel();
        endResetModel();
    }

    // void fetchMore(const QModelIndex &parent) override {
    //     m_searchManager->fetchMore();
    // }
    // bool canFetchMore(const QModelIndex &parent) const override {
    //     return m_searchManager->canFetchMore();
    // }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override {
        if (parent.isValid()) return 0;
        return m_searchManager->count();
    }
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        if (!index.isValid())
            return QVariant();

        const ShowData& show = m_searchManager->getResultAt(index.row());

        switch (role){
        case TitleRole:
            return show.title;
        case CoverRole:
            return show.coverUrl;
        case LinkRole:
            return show.link;
        default:
            break;
        }
        return QVariant();
    }

    QHash<int, QByteArray> roleNames() const override {
        QHash<int, QByteArray> names;
        names[TitleRole] = "title";
        names[CoverRole] = "cover";
        names[LinkRole] = "link";
        return names;
    };
private:
    SearchManager *m_searchManager;
};



