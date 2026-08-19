#pragma once
#include <QAbstractListModel>
#include <QAbstractItemModel>
#include <qqmlintegration.h>
#include "net/canceltoken.h"

// Qt-model bases adding the shared cancellation token; isLoading lives per-manager.
class ListModel : public QAbstractListModel {
    Q_OBJECT
    QML_ANONYMOUS
public:
    explicit ListModel(QObject *parent = nullptr) : QAbstractListModel(parent) {}
protected:
    CancelToken m_cancel;
};

class TreeModel : public QAbstractItemModel {
    Q_OBJECT
    QML_ANONYMOUS
public:
    explicit TreeModel(QObject *parent = nullptr) : QAbstractItemModel(parent) {}
protected:
    CancelToken m_cancel;
};
