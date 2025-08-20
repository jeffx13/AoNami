#include "providermanager.h"
#include "providers/showprovider.h"

ProviderManager::ProviderManager(QObject *parent)
    : QAbstractListModel(parent)
{}

void ProviderManager::setCurrentProviderIndex(int index) {
    if (index == m_currentProviderIndex) return;
    if (m_providers.isEmpty()) return;
    if (index < 0 || index >= m_providers.size()) return;

    const QString currentSearchType = (m_availableTypes.isEmpty() || m_currentSearchTypeIndex < 0 || m_currentSearchTypeIndex >= m_availableTypes.size())
        ? QString()
        : m_availableTypes.at(m_currentSearchTypeIndex);

    m_currentProviderIndex = index;
    m_currentSearchProvider = m_providers.at(index);
    m_availableTypes = m_currentSearchProvider ? m_currentSearchProvider->getAvailableTypes() : QStringList{};

    emit currentProviderIndexChanged();

    const int searchTypeIndex = m_availableTypes.indexOf(currentSearchType);
    m_currentSearchTypeIndex = searchTypeIndex == -1 ? 0 : searchTypeIndex;
    emit currentSearchTypeIndexChanged();
}

void ProviderManager::setCurrentSearchTypeIndex(int index) {
    if (index == m_currentSearchTypeIndex) return;
    if (index < 0 || index >= m_availableTypes.size()) return;
    m_currentSearchTypeIndex = index;
    emit currentSearchTypeIndexChanged();
}

void ProviderManager::setProviders(QList<ShowProvider*> &&providers) {
    beginResetModel();
    m_providers = providers;
    m_providersMap.clear();
    Q_FOREACH(ShowProvider* provider, m_providers) {
        if (!provider) continue;
        m_providersMap.insert(provider->name(), provider);
    }
    m_currentProviderIndex = -1;
    m_currentSearchTypeIndex = 0;
    m_currentSearchProvider = nullptr;
    m_availableTypes.clear();
    endResetModel();

    if (!m_providers.isEmpty()) setCurrentProviderIndex(0);
}

void ProviderManager::cycleProviders() {
    if (m_providers.isEmpty()) return;
    const int next = (m_currentProviderIndex + 1) % m_providers.count();
    setCurrentProviderIndex(next);
}

int ProviderManager::rowCount(const QModelIndex &parent) const{
    Q_UNUSED(parent);
    return m_providers.count();
}

QVariant ProviderManager::data(const QModelIndex &index, int role) const{
    if (!index.isValid()) return QVariant();
    if (index.row() < 0 || index.row() >= m_providers.size()) return QVariant();
    ShowProvider* provider = m_providers.at(index.row());
    if (!provider) return QVariant();
    switch (role){
    case NameRole:
        return provider->name();

    default:
        break;
    }
    return QVariant();
}

QHash<int, QByteArray> ProviderManager::roleNames() const{
    QHash<int, QByteArray> names;
    names[NameRole] = "text";
    // names[IconRole] = "icon";
    return names;
}

void ProviderManager::setCurrentProviderByName(const QString &providerName) {
    if (providerName.isEmpty()) return;
    for (int i = 0; i < m_providers.size(); ++i) {
        ShowProvider* p = m_providers.at(i);
        if (p && p->name() == providerName) {
            setCurrentProviderIndex(i);
            return;
        }
    }
}
