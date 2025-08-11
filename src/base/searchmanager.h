#pragma once
#include <QAbstractListModel>
#include <QFutureWatcher>
#include "base/network/network.h"
#include "showdata.h"
#include "base/servicemanager.h"


class SearchManager: public ServiceManager
{
    Q_OBJECT
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)
    Q_PROPERTY(float contentY READ getContentY WRITE setContentY NOTIFY contentYChanged)
public:
    explicit SearchManager(QObject *parent = nullptr);
    ShowData &getResultAt(int index) { return m_list[index]; }


    void search(const QString& query,int page,int type, ShowProvider* provider);
    void latest(int page, int type, ShowProvider* provider);
    void popular(int page, int type, ShowProvider* provider);


    float getContentY() const;
    void setContentY(float newContentY);
    Q_SIGNAL void contentYChanged();
    Q_SIGNAL void appended(int, int);
    Q_SIGNAL void cleared(int);

    bool canLoadMore() { return !(m_isLoading || m_watcher.isRunning() || !m_canFetchMore); }

    int count() const { return m_list.count(); }
Q_INVOKABLE void cancel();
private:
    QFutureWatcher<QList<ShowData>> m_watcher;
    std::atomic<bool> m_isCancelled = false;
    Client m_client{&m_isCancelled};
    QList<ShowData> m_list;
    QString m_cancelReason;

    int m_currentPage;
    bool m_canFetchMore = false;




    // enum {
    //     TitleRole = Qt::UserRole,
    //     CoverRole
    // };
    // int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    // QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    // QHash<int, QByteArray> roleNames() const override {
    //     QHash<int, QByteArray> names;
    //     names[TitleRole] = "title";
    //     names[CoverRole] = "cover";
    //     return names;
    // };

    float m_contentY;
};
