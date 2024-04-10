#pragma once
#include <QAbstractListModel>
#include <QtConcurrent>

#include "data/showdata.h"



class SearchResultManager : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)
public:
    explicit SearchResultManager(QObject *parent = nullptr);
    ShowData &at(int index) { return m_list[index]; }

    void search(const QString& query,int page,int type, ShowProvider* provider);
    void latest(int page, int type, ShowProvider* provider);
    void popular(int page, int type, ShowProvider* provider);
    Q_INVOKABLE void cancelLoading();
    Q_INVOKABLE void reload();
    Q_SIGNAL void isLoadingChanged(void);
private:
    QFutureWatcher<QList<ShowData>> m_watcher;
    QList<ShowData> m_list;
    QString m_cancelReason;
    int m_currentPage;
    bool m_canFetchMore = false;
    std::function<void(int)> lastSearch;
    bool m_isLoading = false;
    void setIsLoading(bool b) { m_isLoading = b; emit isLoadingChanged(); }
    bool isLoading() { return m_isLoading; }

    enum {
        TitleRole = Qt::UserRole,
        CoverRole
    };
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override {
        QHash<int, QByteArray> names;
        names[TitleRole] = "title";
        names[CoverRole] = "cover";
        return names;
    };

    void fetchMore(const QModelIndex &parent) override {
        if (m_watcher.isRunning()) return;
        lastSearch(m_currentPage + 1);
    };
    bool canFetchMore(const QModelIndex &parent) const override {
        return (m_isLoading || m_watcher.isRunning()) ? false : m_canFetchMore;
    };
};

