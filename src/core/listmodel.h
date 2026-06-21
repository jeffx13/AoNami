#pragma once
#include <QAbstractListModel>
#include <QAbstractItemModel>
#include "core/network/canceltoken.h"

// Qt-model bases adding the shared cancellation token; isLoading lives per-manager.
class ListModel : public QAbstractListModel {
public:
    explicit ListModel(QObject *parent = nullptr) : QAbstractListModel(parent) {}
protected:
    CancelToken m_cancel;
};

class TreeModel : public QAbstractItemModel {
public:
    explicit TreeModel(QObject *parent = nullptr) : QAbstractItemModel(parent) {}
protected:
    CancelToken m_cancel;
};
