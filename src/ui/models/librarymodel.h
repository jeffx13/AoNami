#pragma once
#include <QAbstractListModel>
#include "forwards.h"

class LibraryModel: public QAbstractListModel
{
public:
    explicit LibraryModel(LibraryManager *libraryManager);
    enum {
        TitleRole = Qt::UserRole,
        CoverRole,
        UnwatchedEpisodesRole,
        TypeRole,
    };

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
private:
    LibraryManager *m_libraryManager;

};
