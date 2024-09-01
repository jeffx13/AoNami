#include "providermanager.h"
#include "providers/iyf.h"
// #include "Providers/testprovider.h"
#include "providers/kimcartoon.h"
#include "providers/gogoanime.h"
// #include "Providers/nivod.h"
#include "providers/haitu.h"
#include "providers/allanime.h"
#include "providers/wolong.h"
#include "providers/broken/fmovies.h"

ProviderManager::ProviderManager(QObject *parent)
    : QAbstractListModel(parent)
{
    m_providers =
        {
            // new Nivod,
            // new FMovies,
            new AllAnime,
            new Gogoanime,
            new Haitu,
            new Wolong,
            new Kimcartoon,
            new IyfProvider,

            // #ifdef QT_DEBUG
            //             new TestProvider
            // #endif
        };

    for (ShowProvider* provider : m_providers) {
        m_providersMap.insert(provider->name(), provider);
    }

    setCurrentProviderIndex(0);


}

void ProviderManager::setCurrentProviderIndex(int index) {
    if (index == m_currentProviderIndex) return;
    m_currentSearchType = m_availableTypes.isEmpty() ? -1 : m_availableTypes[m_currentSearchTypeIndex];
    m_currentProviderIndex = index;
    m_currentSearchProvider = m_providers.at (index);
    m_availableTypes = m_currentSearchProvider->getAvailableTypes();
    emit currentProviderIndexChanged();
    int searchTypeIndex =  m_availableTypes.indexOf (m_currentSearchType);
    m_currentSearchTypeIndex = searchTypeIndex == -1 ? 0 : searchTypeIndex;
    m_currentSearchType = m_availableTypes[m_currentSearchTypeIndex];
    emit currentSearchTypeIndexChanged();
}

void ProviderManager::setCurrentSearchTypeIndex(int index) {
    if (index == m_currentSearchTypeIndex) return;
    if (index < 0 || index >= m_availableTypes.size()) return;
    m_currentSearchType = m_availableTypes[index];
    m_currentSearchTypeIndex = index;
    emit currentSearchTypeIndexChanged();
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
        break;
    // case IconRole:
    // break;
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
