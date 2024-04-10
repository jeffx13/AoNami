#pragma once
#include <QAbstractListModel>

class ShowProvider;

class ProviderManager : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int      currentProviderIndex   READ getCurrentProviderIndex   WRITE setCurrentProviderIndex   NOTIFY currentProviderIndexChanged)
    Q_PROPERTY(int      currentSearchTypeIndex READ getCurrentSearchTypeIndex WRITE setCurrentSearchTypeIndex NOTIFY currentSearchTypeIndexChanged)
    Q_PROPERTY(QVariant availableShowTypes     READ getAvailableShowTypes                                     NOTIFY currentProviderIndexChanged)


public:
    explicit ProviderManager(QObject *parent = nullptr);
    ~ProviderManager() { qDeleteAll (m_providers); }
    Q_INVOKABLE void cycleProviders();
    ShowProvider *getCurrentSearchProvider() const { return m_currentSearchProvider; }
    int getCurrentSearchType() const { return m_currentSearchType; }
    ShowProvider *getProvider(const QString& providerName) const {
        if (!m_providersMap.contains (providerName)) return nullptr;
        return m_providersMap[providerName];
    }
signals:
    void currentSearchTypeIndexChanged(void);
    void currentProviderIndexChanged(void);
private:
    QList<ShowProvider*> m_providers;
    QHash<QString, ShowProvider*> m_providersMap;
    ShowProvider *m_currentSearchProvider;

    int getCurrentProviderIndex() const { return m_currentProviderIndex; }
    void setCurrentProviderIndex(int index);
    int m_currentProviderIndex = -1;
    void setCurrentSearchTypeIndex(int index);
    int getCurrentSearchTypeIndex() const { return m_currentSearchTypeIndex; }
    int m_currentSearchTypeIndex = 0;
    int m_currentSearchType = -1;
    QList<int> m_availableTypes;
    QVariant getAvailableShowTypes() {
        QStringList stringTypes = {"Movie", "Tv Series", "Variety", "Anime", "Documentary", "None"};
        QStringList availableTypes;
        for (auto type : m_availableTypes){
            availableTypes.push_back (stringTypes[type - 1]);
        }
        return QVariant::fromValue(availableTypes);
    }

    enum {
        NameRole = Qt::UserRole,
        // IconRole, //TODO add icons next to providers
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
private:
};


