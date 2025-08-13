#include "librarymanager.h"
#include "providermanager.h"
#include "providers/showprovider.h"
int LibraryManager::count(int listType) const {
    if (listType == -1)
        listType = m_currentListType;
    return m_watchListJson[listType].toArray().size();
}

bool LibraryManager::loadFile(const QString &filePath) {

    if (m_updatedByApp == 1) {
        m_updatedByApp++;
        return false;
    } else if (m_updatedByApp == 2) {
        m_updatedByApp = 0;
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
    emit aboutToInsert(0, count());
    for (int type = 0; type < m_watchListJson.size(); ++type) {
        const QJsonArray& array = m_watchListJson.at(type).toArray();
        for (int index = 0; index < array.size(); ++index) {
            const QJsonObject& show = array.at(index).toObject();
            QString link = show["link"].toString();
            m_showHashmap.insert(link, {type, index, -1});
        }
    }
    emit inserted();
    cLog() << "Library" << "Loaded" << libraryPath;
    return true;
}

void LibraryManager::updateProperty(const QString &showLink, const QList<Property>& properties){
    if (!m_showHashmap.contains(showLink)) return;

    auto showHelperInfo = m_showHashmap.value(showLink);

    QJsonArray list = m_watchListJson[showHelperInfo.listType].toArray();
    QJsonObject show = list[showHelperInfo.index].toObject();

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


    list[showHelperInfo.index] = show; // Update the show in the list
    m_watchListJson[showHelperInfo.listType] = list; // Update the list in the model
    save(); // Save changes
}


void LibraryManager::updateProgress(const QString &showLink, int lastWatchedIndex, int timeStamp) {
    if (!m_showHashmap.contains(showLink)) return;
    updateProperty(showLink, {
                                 {"lastWatchedIndex", lastWatchedIndex, Property::INT},
                                 {"timeStamp", timeStamp, Property::INT}
                             });
    if(m_showHashmap[showLink].listType == m_currentListType) {
        int showIndex = m_showHashmap[showLink].index;
        emit changed(showIndex);
    }
}

ShowData::LastWatchInfo LibraryManager::getLastWatchInfo(const QString &showLink) {
    ShowData::LastWatchInfo info;
    if (!m_showHashmap.contains(showLink)) return info;

    // Retrieve the list type and index for the show
    auto showLibInfo = m_showHashmap[showLink];
    QJsonArray list = m_watchListJson.at(showLibInfo.listType).toArray();
    QJsonObject showObject = list.at(showLibInfo.index).toObject();

    // Sync details
    info.listType = showLibInfo.listType;
    info.lastWatchedIndex = showObject["lastWatchedIndex"].toInt(-1);
    info.timeStamp = showObject["timeStamp"].toInt(0);

    return info;
}

QJsonObject LibraryManager::getShowJsonAt(int index) const {
    // Validate the current list type and index
    // if (mapped) {
    //     auto const proxyIndex = m_proxyModel.index(index, 0);
    //     index = m_proxyModel.mapToSource(proxyIndex).row();
    // }

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
    if (m_showHashmap.contains(show.link)) {
        changeListType(show.link, listType);
        return;
    }

    QJsonObject showJson = show.toJsonObject();

    // Append the new show to the appropriate list
    QJsonArray list = m_watchListJson.at(listType).toArray();
    if (m_currentListType == listType) {
        emit aboutToInsert(list.size() - 1, 1);
    }
    list.append(showJson);
    m_watchListJson[listType] = list; // Update the list in m_jsonList

    // Update the hashmap
    m_showHashmap[show.link] = ShowLibInfo{ listType, (int)list.count() - 1, -1 };


    // Model update signals
    if (m_currentListType == listType) {
        emit inserted();
    }
    save();
}

void LibraryManager::removeByLink(const QString &link) {
    if (!m_showHashmap.contains(link)) return;
    // Extract list type and index from the hashmap
    auto libInfo = m_showHashmap.value(link);
    removeAt(libInfo.index, libInfo.listType);
}

void LibraryManager::removeAt(int index, int listType) {
    if (listType < 0 || listType > 4) listType = m_currentListType;
    QJsonArray list = m_watchListJson[listType].toArray();

    if (index < 0 || index >= list.size()) {
        qCritical() << "Error removing show: invalid index" << index;
        return;
    }

    auto showLink = list[index].toObject()["link"].toString();

    if (m_currentListType == listType) {
        emit aboutToRemove(index);
    }
    list.removeAt(index);
    m_watchListJson[listType] = list;

    if (m_currentListType == listType) {
        emit removed();
    }

    m_showHashmap.remove(showLink);

    for (int i = index; i < list.size(); ++i) {
        QJsonObject show = list[i].toObject();
        QString showLink = show["link"].toString();
        auto &showHelperInfo = m_showHashmap[showLink];
        showHelperInfo.index = i;
    }
    save();
}

void LibraryManager::move(int from, int to) {
    // Validate the 'from' and 'to' positions
    if (from == to || from < 0 || to < 0) return;

    QJsonArray currentList = m_watchListJson.at(m_currentListType).toArray();
    if (from >= currentList.size() || to >= currentList.size()) return;


    // emit aboutToMove(from, to);
    emit aboutToRemove(from);
    QJsonObject movingShow = currentList.takeAt (from).toObject();
    emit removed();
    emit aboutToInsert(to, 1);
    currentList.insert(to, movingShow);
    // emit moved();
    emit inserted();
    m_watchListJson[m_currentListType] = currentList;

    for (int i = qMin(from, to); i <= qMax(from, to); ++i) {
        QJsonObject show = currentList.at(i).toObject();
        QString showLink = show["link"].toString();
        m_showHashmap[showLink].index = i;
    }


    save();
}

void LibraryManager::changeListTypeAt(int index, int newListType, int oldListType) {
    oldListType = oldListType == -1 ? m_currentListType : oldListType;
    if (oldListType == newListType) return;

    QJsonArray oldList = m_watchListJson[oldListType].toArray();
    QJsonArray newList = m_watchListJson[newListType].toArray();

    if (index < 0 || index >= oldList.size()) {
        qCritical() << "Error changing list type: invalid source index" << index;
        return;
    }

    if (m_currentListType == oldListType) {
        emit aboutToRemove(index);
    }
    QJsonObject showToMove = oldList.takeAt(index).toObject();
    if (m_currentListType == oldListType) {
        emit removed();
    }

    QString showLink = showToMove["link"].toString();




    // Add to new list
    int newIndex = newList.size();

    if (m_currentListType == newListType) {
        emit aboutToInsert(newIndex, 1);
    }
    newList.append(showToMove);
    if (m_currentListType == newListType) {
        emit inserted();
    }


    // Update the JSON structure
    m_watchListJson[oldListType] = oldList;
    m_watchListJson[newListType] = newList;


    // Update the hashmap indices after removal in old list
    for (int i = index; i < oldList.size(); ++i) {
        QJsonObject show = oldList.at(i).toObject();
        QString link = show["link"].toString();
        m_showHashmap[link].index = i;
    }

    // Update the hashmap for the newly added show
    m_showHashmap[showLink].listType = newListType;
    m_showHashmap[showLink].index = newIndex;

    save();
}

void LibraryManager::changeListType(const QString &link, int newListType) {
    if (!m_showHashmap.contains(link)) return;
    auto showLibInfo = m_showHashmap.value(link);
    changeListTypeAt(showLibInfo.index, newListType, showLibInfo.listType);
}

void LibraryManager::fetchUnwatchedEpisodes(int listType) {
    if (m_fetchUnwatchedEpisodesJob.isRunning()) {
        m_isCancelled = true;
        m_fetchUnwatchedEpisodesJob.waitForFinished();
    }
    if (listType < 0 || listType > 4) return;

    m_fetchUnwatchedEpisodesJob = QtConcurrent::run([this, listType] {
        auto shows = m_watchListJson[listType].toArray();

        QList<QFuture<void>> jobs;
        for (int i = 0; i < shows.size(); i++) {
            if (m_isCancelled) return;

            auto showObject = shows[i].toObject();
            auto providerName = showObject["provider"].toString();
            auto provider = ProviderManager::getProvider(providerName);
            if (!provider) continue;

            jobs.push_back(QtConcurrent::run([this, showObject, provider, listType] {
                auto show = ShowData::fromJson(showObject);
                auto client = Client(&m_isCancelled, false);
                if (m_isCancelled) return;
                try {
                    int episodes = provider->loadDetails(&client, show, true, false, false);
                    auto showLink = showObject["link"].toString();
                    m_showHashmap[showLink].totalEpisodes = episodes - 1;
                    if (listType == m_currentListType) {
                        int showIndex = m_showHashmap[showLink].index;
                    }
                } catch (MyException &e) {
                    e.print();
                }
            }));
        }

        for (auto &job: jobs) {
            job.waitForFinished();
        }
        emit fetchedAllEpCounts();
        m_isCancelled = false;

    });


}

void LibraryManager::updateShowCover(const ShowData &show) {
    if (!m_showHashmap.contains(show.link)) return;
    auto showHelperInfo = m_showHashmap.value(show.link);
    int listType = showHelperInfo.listType;
    int showIndex = showHelperInfo.index;
    QJsonArray list = m_watchListJson[listType].toArray();
    QJsonObject showJson = list[showIndex].toObject();
    if (show.coverUrl == showJson["cover"].toString()) {
        return;
    }
    showJson["cover"] = show.coverUrl;
    list[showIndex] = showJson;
    m_watchListJson[listType] = list;
    if (listType == m_currentListType) {
        emit changed(showIndex);
        // emit dataChanged(index(showIndex), index(showIndex), {CoverRole});
    }
    save();
}

void LibraryManager::setDisplayingListType(int newListType) {
    auto oldListType = m_currentListType;
    if (oldListType == newListType) return;
    m_currentListType = newListType;
    emit cleared(count(oldListType));
    // emit inserted(0, count(newListType));
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



