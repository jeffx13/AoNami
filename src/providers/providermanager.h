#pragma once
#include <QAbstractListModel>
#include <QStringList>

class ShowProvider;

class ProviderManager : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int      currentProviderIndex   READ getCurrentProviderIndex   WRITE setCurrentProviderIndex   NOTIFY currentProviderIndexChanged)
    Q_PROPERTY(int      currentSearchTypeIndex READ getCurrentSearchTypeIndex WRITE setCurrentSearchTypeIndex NOTIFY currentSearchTypeIndexChanged)
    Q_PROPERTY(QVariant availableShowTypes     READ getAvailableShowTypes                                     NOTIFY currentProviderIndexChanged)
public:
    explicit ProviderManager(QObject *parent = nullptr);
    ~ProviderManager() { qDeleteAll(m_providers); }

    void setProviders(QList<ShowProvider *> &&providers);

    Q_INVOKABLE void cycleProviders();
    Q_INVOKABLE void setCurrentProviderByName(const QString &providerName);

    ShowProvider *getCurrentSearchProvider() const { return m_currentSearchProvider; }
    int getCurrentSearchType() const { return m_currentSearchTypeIndex; }

    static ShowProvider *getProvider(const QString &providerName) {
        auto it = s_providersMap.constFind(providerName);
        return it != s_providersMap.constEnd() ? it.value() : nullptr;
    }

signals:
    void currentProviderIndexChanged();
    void currentSearchTypeIndexChanged();

private:
    QList<ShowProvider *> m_providers;
    inline static QHash<QString, ShowProvider *> s_providersMap;
    ShowProvider *m_currentSearchProvider = nullptr;
    int m_currentProviderIndex = -1;
    int m_currentSearchTypeIndex = 0;
    QStringList m_availableTypes;

    int getCurrentProviderIndex() const { return m_currentProviderIndex; }
    void setCurrentProviderIndex(int index);
    int getCurrentSearchTypeIndex() const { return m_currentSearchTypeIndex; }
    void setCurrentSearchTypeIndex(int index);
    QVariant getAvailableShowTypes() const {
        return QVariant::fromValue(m_availableTypes.isEmpty() ? QStringList{"All"} : m_availableTypes);
    }

    enum { NameRole = Qt::UserRole };
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
};
