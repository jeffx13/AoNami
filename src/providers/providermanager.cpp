#include "providers/providermanager.h"
#include "providers/showprovider.h"

ProviderManager::ProviderManager(QObject *parent)
    : QAbstractListModel(parent)
{}

void ProviderManager::setProviders(QList<ShowProvider *> &&providers) {
    beginResetModel();
    m_providers = std::move(providers);
    s_providersMap.clear();
    for (ShowProvider *provider : std::as_const(m_providers)) {
        if (provider)
            s_providersMap.insert(provider->name(), provider);
    }
    m_currentProviderIndex = -1;
    m_currentSearchTypeIndex = 0;
    m_currentSearchProvider = nullptr;
    m_availableTypes.clear();
    endResetModel();

    if (!m_providers.isEmpty())
        setCurrentProviderIndex(0);
}

void ProviderManager::setCurrentProviderIndex(int index) {
    if (index == m_currentProviderIndex) return;
    if (m_providers.isEmpty() || index < 0 || index >= m_providers.size()) return;

    const QString prevSearchType =
        (m_currentSearchTypeIndex >= 0 && m_currentSearchTypeIndex < m_availableTypes.size())
            ? m_availableTypes.at(m_currentSearchTypeIndex)
            : QString();

    m_currentProviderIndex = index;
    m_currentSearchProvider = m_providers.at(index);
    m_availableTypes = m_currentSearchProvider->getAvailableTypes();
    emit currentProviderIndexChanged();

    int newTypeIndex = prevSearchType.isEmpty() ? -1 : m_availableTypes.indexOf(prevSearchType);
    m_currentSearchTypeIndex = (newTypeIndex == -1) ? 0 : newTypeIndex;
    emit currentSearchTypeIndexChanged();
}

void ProviderManager::setCurrentSearchTypeIndex(int index) {
    if (index == m_currentSearchTypeIndex) return;
    if (index < 0 || index >= m_availableTypes.size()) return;
    m_currentSearchTypeIndex = index;
    emit currentSearchTypeIndexChanged();
}

void ProviderManager::cycleProviders() {
    if (m_providers.isEmpty()) return;
    setCurrentProviderIndex((m_currentProviderIndex + 1) % m_providers.count());
}

void ProviderManager::setCurrentProviderByName(const QString &providerName) {
    if (providerName.isEmpty()) return;
    ShowProvider *p = s_providersMap.value(providerName, nullptr);
    if (!p) return;
    int idx = m_providers.indexOf(p);
    if (idx >= 0) setCurrentProviderIndex(idx);
}

int ProviderManager::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent);
    return m_providers.count();
}

QVariant ProviderManager::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_providers.size())
        return {};
    ShowProvider *provider = m_providers.at(index.row());
    if (!provider) return {};
    switch (role) {
    case NameRole: return provider->name();
    default:       return {};
    }
}

QHash<int, QByteArray> ProviderManager::roleNames() const {
    return {{NameRole, "text"}};
}
