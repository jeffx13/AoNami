#pragma once
#include <QAbstractListModel>
#include <QtConcurrent>

#include "showdata.h"



class SearchResultManager : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)
    Q_PROPERTY(float contentY READ getContentY WRITE setContentY NOTIFY contentYChanged)
public:
    explicit SearchResultManager(QObject *parent = nullptr);
    ShowData &at(int index) { return m_list[index]; }

    void search(const QString& query,int page,int type, ShowProvider* provider);
    void latest(int page, int type, ShowProvider* provider);
    void popular(int page, int type, ShowProvider* provider);
    Q_INVOKABLE void cancelLoading();
    Q_INVOKABLE void reload();
    Q_SIGNAL void isLoadingChanged(void);
    Q_INVOKABLE void loadMore();;
    float getContentY() const;
    void setContentY(float newContentY);
    Q_SIGNAL void contentYChanged();

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

    float m_contentY;
};
