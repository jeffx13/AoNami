#include "playlistitem.h"
#include "utils/logger.h"



PlaylistItem::PlaylistItem(int seasonNumber, float number, const QString &link, const QString &name, PlaylistItem *parent, bool isLocal)
    : seasonNumber(seasonNumber), number(number), name(name), link(link), m_parent(parent), type(isLocal ? LOCAL : ONLINE) {
    if (number > -1) {
        int dp = number == floor(number) ? 0 : 1;
        QString episodeNumber = QString::number(number, 'f', dp);
        QString season = seasonNumber != 0 ? QString("Season %1 ").arg(seasonNumber) : "";
        fullName = QString("%1Ep. %2\n%3").arg(season, episodeNumber, name);
    } else {
        fullName = name.isEmpty() ? "[Unnamed Episode]" : name;
    }

    if(parent) useCount++;
}

void PlaylistItem::emplaceBack(int seasonNumber, float number, const QString &link, const QString &name, bool isLocal) {
    createChildren();
    auto playlistItem = new PlaylistItem(seasonNumber, number, link, name, this, isLocal);
    m_children->push_back(playlistItem);
}

void PlaylistItem::clear() {
    if (m_children) {
        for (auto &playlist : *m_children) {
            playlist->m_parent = nullptr;
            if (--playlist->useCount == 0)
                delete playlist;
        }
        m_children->clear();
    }
}

void PlaylistItem::removeAt(int index) {
    auto toRemove = at(index);
    if (!m_children || !toRemove) return;
    checkDelete(toRemove);
    m_children->removeAt (index);
}

void PlaylistItem::insert(int index, PlaylistItem *value) {
    if (index == 0 || isValidIndex(index)) {
        createChildren();
        value->useCount++;
        value->m_parent = this;
        m_children->insert(index, value);
    }
}

int PlaylistItem::indexOf(const QString &link) {
    if (!m_children) return -1;
    for (int i = 0; i < m_children->size(); i++) {
        auto child = m_children->at(i);
        if (child->link == link) {
            return i;
        }
    }
    return -1;
}

void PlaylistItem::append(PlaylistItem *value) {
    createChildren();
    value->useCount++;
    value->m_parent = this;
    m_children->push_back(value);
}

void PlaylistItem::removeOne(PlaylistItem *value) {
    if (!m_children || !value) return;
    m_children->removeOne(value);
    checkDelete(value);

}

void PlaylistItem::reverse() {
    if (!m_children || m_children->isEmpty()) return;
    std::reverse(m_children->begin(), m_children->end());
}

QString PlaylistItem::getDisplayNameAt(int index) const {
    if (!isValidIndex(index))
        return "";
    auto currentItem = m_children->at(index);
    QString itemName = "%1\n[%2/%3] %4";
    itemName = itemName.arg(name)
                   .arg(index + 1)
                   .arg(m_children->count())
                   .arg(currentItem->fullName);
    return itemName;
}

void PlaylistItem::updateHistoryFile() {
    if (!m_isLoadedFromFolder || !isValidIndex(currentIndex)) return;

    static QMutex mutex;
    mutex.lock();
    if (m_historyFile->isOpen() || m_historyFile->open(QIODevice::WriteOnly)) {
        m_historyFile->resize(0);
        QTextStream stream(m_historyFile.get());
        QString lastWatchedFilePath = m_children->at(currentIndex)->link;
        stream << lastWatchedFilePath.split("/").last();
        if (timeStamp > 0) {
            stream << ":" << QString::number(timeStamp);
        }
        m_historyFile->close();
    }
    mutex.unlock();
}

void PlaylistItem::setLastPlayAt(int index, int time) {
    if (!isValidIndex(index)) return;
    cLog() << "Playlist" << name << "| Index:" << index << "| Timestamp:" << time;
    currentIndex = index;
    m_children->at(index)->timeStamp = time;
}

void PlaylistItem::checkDelete(PlaylistItem *value) {
    value->m_parent = nullptr;
    if (--value->useCount == 0) {
        delete value;
    }
}

void PlaylistItem::createChildren() {
    if (m_children) return;
    m_children = std::unique_ptr<QList<PlaylistItem*>>(new QList<PlaylistItem*>);
}

bool PlaylistItem::isValidIndex(int index) const {
    if (m_children != nullptr){
        if (m_children->isEmpty()) return false;
        return index >= 0 && index < m_children->size();
    }
    return false;

}
