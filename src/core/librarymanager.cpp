#include "librarymanager.h"


bool LibraryManager::loadFile(const QString &filePath) {

    if (m_updatedByApp == true) {
        m_updatedByApp++;
        return false;
    } else if (m_updatedByApp == 2) {
        m_updatedByApp = false;
        return false;
    }
    QString libraryPath = filePath.isEmpty() ? m_defaultLibraryPath : filePath;

    QFile file(libraryPath);
    if (!file.exists()) {
        if (libraryPath == m_defaultLibraryPath) {
            QFile defaultLibraryFile(m_defaultLibraryPath);
            if (file.open(QIODevice::WriteOnly)) {
                m_showHashmap.clear();
                m_watchListJson = QJsonArray({QJsonArray(), QJsonArray(), QJsonArray(), QJsonArray(), QJsonArray()});
                QJsonDocument doc(m_watchListJson);
                file.write(doc.toJson());
                m_currentLibraryPath = m_defaultLibraryPath;
                m_watchListFileWatcher.addPath(m_currentLibraryPath);
                return true;
            }
        }
        return false;
    }

    if (libraryPath != m_currentLibraryPath) {
        if (!m_currentLibraryPath.isEmpty()) m_watchListFileWatcher.removePath(m_currentLibraryPath);
        m_watchListFileWatcher.addPath(libraryPath);
        m_currentLibraryPath = libraryPath;
    }
    // Attempt to open the file, create and initialize with empty structure if it doesn't exist
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray jsonData = file.readAll();
        file.close();
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(jsonData, &error);
        if (error.error == QJsonParseError::NoError && doc.isArray()) {
            m_watchListJson = doc.array();
            if (m_watchListJson.size() != 5){
                m_watchListJson = QJsonArray();
                return false;
            }
        } else {
            qWarning() << "JSON parsing error:" << error.errorString();
            m_watchListJson = QJsonArray();
            return false;
        }
    } else {
        qWarning() << "Failed to open library file";
        return false;
    }

    // Populate the hash map from the loaded or initialized JSON array
    m_showHashmap.clear();
    for (int type = 0; type < m_watchListJson.size(); ++type) {
        const QJsonArray& array = m_watchListJson.at(type).toArray();
        for (int index = 0; index < array.size(); ++index) {
            const QJsonObject& show = array.at(index).toObject();
            QString link = show["link"].toString();
            m_showHashmap.insert(link, {type, index});
        }
    }

    emit layoutChanged();
    return true;
}

void LibraryManager::updateProperty(const QString &showLink, const QList<Property>& properties){
    if (!m_showHashmap.contains(showLink)) return;

    QPair<int, int> listTypeAndIndex = m_showHashmap.value(showLink);
    int listType = listTypeAndIndex.first;
    int index = listTypeAndIndex.second;

    QJsonArray list = m_watchListJson[listType].toArray();
    QJsonObject show = list[index].toObject();

    for (const auto& property: properties) {
        switch (property.type) {
        case Property::INT:
            show.operator[](property.name) = property.value.toInt();
            break;
        case Property::FLOAT:
            show.operator[](property.name) = property.value.toFloat();
            break;
        case Property::STRING:
            show.operator[](property.name) = property.value.toString();
            break;
        }
    }


    list[index] = show; // Update the show in the list
    m_watchListJson[listType] = list; // Update the list in the model
    save(); // Save changes
}

LibraryProxyModel* LibraryManager::getProxyModel()
{
    return &m_proxyModel;
}

void LibraryManager::updateLastWatchedIndex(const QString &showLink, int lastWatchedIndex) {
    updateProperty(showLink, {
                                 {"lastWatchedIndex", lastWatchedIndex, Property::INT},
                                 {"timeStamp", 0, Property::INT}
                             });
}

void LibraryManager::updateTimeStamp(const QString &showLink, int timeStamp) {
    updateProperty(showLink, {{"timeStamp", timeStamp, Property::INT}});
}

ShowData::LastWatchInfo LibraryManager::getLastWatchInfo(const QString &showLink) {
    ShowData::LastWatchInfo info;
    // Check if the show exists in the hashmap
    if (m_showHashmap.contains (showLink)) {
        // Retrieve the list type and index for the show
        QPair<int, int> listTypeAndIndex = m_showHashmap.value(showLink);
        int listType = listTypeAndIndex.first;
        int index = listTypeAndIndex.second;
        QJsonArray list = m_watchListJson.at(listType).toArray();
        QJsonObject showObject = list.at(index).toObject();

        // Sync details
        info.listType = listType;
        info.lastWatchedIndex = showObject["lastWatchedIndex"].toInt(-1);
        info.timeStamp = showObject["timeStamp"].toInt(0);
    }
    return info;
}

QJsonObject LibraryManager::getShowJsonAt(int index, bool mapped) const {
    // Validate the current list type and index
    if (mapped) {
        auto const proxyIndex = m_proxyModel.index(index, 0);
        index = m_proxyModel.mapToSource(proxyIndex).row();
    }

    const QJsonArray& currentList = m_watchListJson.at(m_currentListType).toArray();
    if (index < 0 || index >= currentList.size()) {
        qWarning() << "Index out of bounds for the current list";
        return QJsonObject(); // Return an empty object for invalid index or list type
    }
    // Retrieve and return the show object at the given index
    return currentList.at(index).toObject();
}

void LibraryManager::add(ShowData& show, int listType)
{
    // Check if the show is already in the library, and if so, change its list type
    if (m_showHashmap.contains (show.link)) {
        changeShowListType (show, listType);
    } else {
        // Convert ShowData to QJsonObject
        QJsonObject showJson = show.toJson();

        // Append the new show to the appropriate list
        QJsonArray list = m_watchListJson.at(listType).toArray();
        list.append(showJson);
        m_watchListJson[listType] = list; // Update the list in m_jsonList

        // Update the hashmap
        m_showHashmap[show.link] = qMakePair(listType, list.count() - 1);

        // Model update signals
        if (m_currentListType == listType) {
            int newSize = list.size();
            beginInsertRows(QModelIndex(), newSize, newSize);
            endInsertRows();
        }
        show.setListType (listType);
        save();
    }
}

void LibraryManager::remove(ShowData &show)
{
    // QString showLink = QString::fromStdString(show.link);
    // Check if the show exists in the hashmap
    if (!m_showHashmap.contains(show.link)) return;

    // Extract list type and index from the hashmap
    QPair<int, int> listTypeAndIndex = m_showHashmap.value(show.link);
    int listType = listTypeAndIndex.first;
    int index = listTypeAndIndex.second;

    removeAt (index, listType);
    show.setListType (-1);
}

void LibraryManager::removeAt(int index, int listType) {
    if (listType < 0 || listType > 4) listType = m_currentListType;
    QJsonArray list = m_watchListJson[listType].toArray();

    if (index < 0 || index >= list.size()) return;
    index = m_proxyModel.mapToSource(m_proxyModel.index(index, 0)).row();

    auto showLink = list[index].toObject()["link"].toString();

    // Remove the show from the list
    list.removeAt(index);
    m_watchListJson[listType] = list; // Update the list in the JSON structure

    // Remove the show from the hashmap
    m_showHashmap.remove(showLink);

    for (int i = index; i < list.size(); ++i) {
        QJsonObject show = list[i].toObject();
        QString showLink = show["link"].toString();
        m_showHashmap[showLink].second = i; // Update index
    }

    // If the current list type is being displayed, update the model accordingly
    if (m_currentListType == listType) {
        beginRemoveRows(QModelIndex(), index, index);
        endRemoveRows();
    }

    save(); // Save the changes to the JSON file
}

void LibraryManager::cycleDisplayingListType() {
    setDisplayingListType ((m_currentListType + 1) % 5);
}

void LibraryManager::move(int from, int to) {
    // Validate the 'from' and 'to' positions
    if (from == to || from < 0 || to < 0) return;
    from = m_proxyModel.mapToSource(m_proxyModel.index(from, 0)).row();
    to = m_proxyModel.mapToSource(m_proxyModel.index(to, 0)).row();

    QJsonArray currentList = m_watchListJson.at(m_currentListType).toArray();
    if (from >= currentList.size() || to >= currentList.size()) return;

    // Perform the move in the JSON array
    QJsonObject movingShow = currentList.takeAt (from).toObject();
    currentList.insert(to, movingShow);
    m_watchListJson[m_currentListType] = currentList; // Update the JSON structure

    for (int i = qMin(from, to); i <= qMax(from, to); ++i) {
        QJsonObject show = currentList.at(i).toObject();
        QString showLink = show["link"].toString();
        m_showHashmap[showLink].second = i; // Update index
    }
    // emit dataChanged(index(qMax(from, to),0), index(qMin(from, to),0));
    beginMoveRows(QModelIndex(), from, from, QModelIndex(), to > from ? to + 1 : to);
    endMoveRows();
    save(); // Save changes
}

void LibraryManager::save() {
    if (m_watchListJson.isEmpty()) return;
    QFile file(m_currentLibraryPath);

    if (!file.exists()) return;
    QMutexLocker locker(&mutex);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Could not open file for writing:" << m_currentLibraryPath;
        return;
    }

    QJsonDocument doc(m_watchListJson); // Wrap the QJsonArray in a QJsonDocument
    file.write(doc.toJson(QJsonDocument::Indented)); // Write JSON data in a readable format
    file.close();
    m_updatedByApp = true;
}

void LibraryManager::changeShowListType(ShowData &show, int newListType) {
    // Check if the library has the show
    if (!m_showHashmap.contains (show.link)) return;

    // Retrieve the list type and the index of the show in the list
    QPair<int, int> listTypeAndIndex = m_showHashmap.value(show.link);
    int oldListType = listTypeAndIndex.first;
    int index = listTypeAndIndex.second;
    changeListTypeAt (index, newListType, oldListType);
    show.setListType (newListType);
}

void LibraryManager::changeListTypeAt(int index, int newListType, int oldListType) {
    if (oldListType == -1) oldListType = m_currentListType;
    if(oldListType == newListType) return;

    QJsonArray oldList = m_watchListJson[oldListType].toArray();
    QJsonArray newList = m_watchListJson[newListType].toArray();

    // Extract the show to move
    index = m_proxyModel.mapToSource(m_proxyModel.index(index, 0)).row();
    QJsonObject showToMove = oldList.takeAt(index).toObject();
    QString showLink = showToMove["link"].toString();

    // Add to new list
    newList.append(showToMove);

    // Update the JSON structure
    m_watchListJson[oldListType] = oldList;
    m_watchListJson[newListType] = newList;

    // Update the hashmap
    m_showHashmap[showLink] = qMakePair (newListType, newList.count() - 1);

    for (int i = index; i < oldList.size(); ++i) {
        QJsonObject show = oldList.at(i).toObject();
        QString link = show["link"].toString();
        m_showHashmap[link].second = i; // Update index
    }

    // Emit model layout changes if necessary
    if (m_currentListType == oldListType) {
        beginRemoveRows (QModelIndex(), index, index);
        endRemoveRows();
    } else if (m_currentListType == newListType) {
        beginInsertRows(QModelIndex(), newList.size() - 1, newList.size());
        endInsertRows();
    }
    save(); // Save changes
}

void LibraryManager::fetchUnwatchedEpisodes(int listType) {

    // int count = 0;
    // for (const auto& show : m_watchListJson[m_currentListType]) {
    //     QString providerName = QString::fromStdString (show["provider"].get<std::string>());
    //     // auto provider = ShowManagex).getProvider (providerName);
    //     // if (!provider) {
    //     //     qDebug()<<"Unable to find a provider for provider enum" << providerName;
    //     //     continue;
    //     // }
    //     // int totalEpisodes = provider->getTotalEpisodes(show["link"].get<std::string>());
    //     //totalEpisodeMap.insert ({show["link"].get<std::string>(), totalEpisodes)

    //     ++count;
    // }
    // emit layoutChanged();
}



int LibraryManager::rowCount(const QModelIndex &parent) const {
    if (parent.isValid())
        return 0;
    return m_watchListJson[m_currentListType].toArray().size();
}

QVariant LibraryManager::data(const QModelIndex &index, int role) const {
    if (!index.isValid())
        return QVariant();
    try{
        auto show = m_watchListJson[m_currentListType].toArray().at (index.row());
        //    const auto show = m_list[m_currentListType][index.row()];
        switch (role){
        case TitleRole:
            return show["title"].toString();
            break;
        case CoverRole:
            return show["cover"].toString();
            break;
        case TypeRole:
            return show["type"].toInt(0);
            break;
        case UnwatchedEpisodesRole:
            //                if (show->totalEpisodes > show->lastWatchedIndex)
            //                {
            //                    if (show->lastWatchedIndex < 0){
            //                        return show->totalEpisodes;
            //                    }
            //                    return show->totalEpisodes - show->lastWatchedIndex - 1;
            //                }
            break;
        default:
            break;
        }

    }catch(...)
    {
        return {};
    }


    return QVariant();
}

QHash<int, QByteArray> LibraryManager::roleNames() const {
    QHash<int, QByteArray> names;
    names[TitleRole] = "title";
    names[CoverRole] = "cover";
    names[UnwatchedEpisodesRole] = "unwatchedEpisodes";
    return names;
}


