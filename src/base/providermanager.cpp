#include "providermanager.h"
#include "providers/showprovider.h"

ProviderManager::ProviderManager(QObject *parent)
    : QAbstractListModel(parent)
{}

void ProviderManager::setCurrentProviderIndex(int index) {
    if (index == m_currentProviderIndex) return;
    auto currentSearchType = m_availableTypes.isEmpty() ? "" : m_availableTypes[m_currentSearchTypeIndex];
    m_currentProviderIndex = index;
    m_currentSearchProvider = m_providers.at (index);
    m_availableTypes = m_currentSearchProvider->getAvailableTypes();
    emit currentProviderIndexChanged();
    int searchTypeIndex =  m_availableTypes.indexOf(currentSearchType);
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
    m_providers = providers;
    for (ShowProvider* provider : std::as_const(m_providers)) {
        m_providersMap.insert(provider->name(), provider);
    }
    setCurrentProviderIndex(0);
}

void ProviderManager::cycleProviders() {
    setCurrentProviderIndex((m_currentProviderIndex + 1) % m_providers.count());
}

int ProviderManager::rowCount(const QModelIndex &parent) const{
    return m_providers.count();
}

QVariant ProviderManager::data(const QModelIndex &index, int role) const{
    if (!index.isValid())
        return QVariant();
    auto provider = m_providers.at(index.row());
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
