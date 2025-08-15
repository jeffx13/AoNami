#include "playlistitem.h"
#include "app/logger.h"
// #include "app/myexception.h"
// #include "playinfo.h"

PlaylistItem::PlaylistItem(int seasonNumber, float number, const QString &link, const QString &name, PlaylistItem *parent, bool isLocal)
    : seasonNumber(seasonNumber), number(number), name(name), link(link), m_parent(parent), type(isLocal ? LOCAL : ONLINE) {
    if (number > -1) {
        int dp = number == floor(number) ? 0 : 1;
        QString episodeNumber = QString::number(number, 'f', dp);
        QString season = seasonNumber != 0 ? QString("Season %1 ").arg(seasonNumber) : "";
        displayName = QString("%1Ep. %2").arg(season, episodeNumber) + (!name.isEmpty() ? QString("\n%1").arg(name) : "");

    } else {
        displayName = name.isEmpty() ? "[Unnamed Episode]" : name;
    }

    if(parent) m_useCount++;
}

void PlaylistItem::emplaceBack(int seasonNumber, float number, const QString &link, const QString &name, bool isLocal) {
    createChildren();
    auto playlistItem = new PlaylistItem(seasonNumber, number, link, name, this, isLocal);
    m_children->push_back(playlistItem);
}

void PlaylistItem::clear() {
    if (m_children) {
        for (auto &child : *m_children) {
            child->m_parent = nullptr;
            if (--child->m_useCount == 0)
                delete child;
        }
        m_children->clear();
    }
}

void PlaylistItem::removeAt(int index) {
    auto toRemove = at(index);
    if (!m_children || !toRemove) return;
    if (index == m_currentIndex) m_currentIndex = -1;
    checkDelete(toRemove);
    m_children->removeAt(index);
}

void PlaylistItem::insert(int index, PlaylistItem *value) {
    if (index >= 0 && index <= count()) {
        createChildren();
        value->m_useCount++;
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

bool PlaylistItem::setCurrentIndex(int index) {
    if(index != -1 && !isValidIndex(index))
        return false;
    m_currentIndex = index;
    return true;
}

void PlaylistItem::append(PlaylistItem *value) {
    createChildren();
    value->m_useCount++;
    value->m_parent = this;
    m_children->push_back(value);
}

void PlaylistItem::removeOne(PlaylistItem *value) {
    if (!m_children || !value) return;
    auto index = indexOf(value);
    removeAt(index);

}

void PlaylistItem::reverse() {
    if (!m_children || m_children->isEmpty()) return;
    std::reverse(m_children->begin(), m_children->end());
}



void PlaylistItem::updateHistoryFile() {
    if (!m_historyFile || !isValidIndex(m_currentIndex)) return;

    static QMutex mutex;
    mutex.lock();
    if (m_historyFile->isOpen() || m_historyFile->open(QIODevice::WriteOnly)) {
        m_historyFile->resize(0);
        QTextStream stream(m_historyFile.get());
        auto item = m_children->at(m_currentIndex);
        QString filename = item->link.split("/").last();
        auto timestamp = item->m_timestamp;
        stream << filename;
        if (timestamp > 0) {
            stream << ":" << QString::number(timestamp);
        }
        m_historyFile->close();
    }
    mutex.unlock();
}

// void PlaylistItem::setLastPlayAt(int index, int time) {
//     if (!isValidIndex(index)) return;
//     cLog() << "Playlist" << name << "| Index:" << index << "| Timestamp:" << time;
//     currentIndex = index;
//     m_children->at(index)->timeStamp = time;
// }

void PlaylistItem::checkDelete(PlaylistItem *value) {
    value->m_parent = nullptr;
    if (--value->m_useCount == 0) {
        delete value;
    }
}

void PlaylistItem::createChildren() {
    if (m_children) return;
    m_children = std::unique_ptr<QList<PlaylistItem*>>(new QList<PlaylistItem*>);
}

bool PlaylistItem::isValidIndex(int index) const {
    if (!m_children || m_children->isEmpty()) return false;
    return index >= 0 && index < m_children->size();
}
