#pragma once
#include <QAbstractListModel>
#include "base/searchmanager.h"
#include "base/showdata.h"


class SearchResultModel: public QAbstractListModel
{
    Q_OBJECT
public:
    SearchResultModel(SearchManager *searchManager, QObject *parent = nullptr)
        : m_searchManager(searchManager), QAbstractListModel(parent) {

        connect(m_searchManager, &SearchManager::appended, this,
                [this](int startIndex, int count) {
                    beginInsertRows(QModelIndex(), startIndex, startIndex + count - 1);
                    endInsertRows();
                });

        connect(m_searchManager, &SearchManager::cleared, this,
                [this](int oldCount) {
                    beginRemoveRows(QModelIndex(), 0, oldCount - 1);
                    endRemoveRows();
                });
    }
    enum {
        TitleRole = Qt::UserRole,
        CoverRole
    };
    int rowCount(const QModelIndex &parent = QModelIndex()) const override {
        if (parent.isValid())
            return 0;
        return m_searchManager->count();
    }
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        if (!index.isValid())
            return QVariant();

        const ShowData& show = m_searchManager->getResultAt(index.row());

        switch (role){
        case TitleRole:
            return show.title;
            break;
        case CoverRole:
            return show.coverUrl;
            break;
        default:
            break;
        }
        return QVariant();
    }

    QHash<int, QByteArray> roleNames() const override {
        QHash<int, QByteArray> names;
        names[TitleRole] = "title";
        names[CoverRole] = "cover";
        return names;
    };
private:
    SearchManager *m_searchManager;
};



