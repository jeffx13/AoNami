#include "playlistitem.h"
#include <QtGlobal>
#include <QFileInfo>
#include <cmath>
#include <algorithm>

PlaylistItem::PlaylistItem(int seasonNumber, float number, const QString &link, const QString &name, PlaylistItem *parent, bool isLocal)
    : seasonNumber(seasonNumber), number(number), name(name), link(link), m_parent(parent), type(isLocal ? LOCAL : ONLINE) {
    if (number > -1) {
        bool isInt = floorf(number) == number;

        QString season = seasonNumber != 0 ? QString("S%1").arg(seasonNumber, 2, 10, QChar('0')) : "";
        displayName = season + (isInt ? QString("E%1").arg(int(number), 2, 10, QChar('0')) : QString::number(number, 'f', 1)) ;
        if (!name.isEmpty())
            displayName += QString("\n%1").arg(name);
    } else {
        displayName = name.isEmpty() ? "[Unnamed Episode]" : name;
    }

    if(parent) m_useCount++;
}

void PlaylistItem::emplaceBack(int seasonNumber, float number, const QString &link, const QString &name, bool isLocal) {
    auto playlistItem = new PlaylistItem(seasonNumber, number, link, name, this, isLocal);
    m_children.push_back(playlistItem);
}

void PlaylistItem::clear() {
    if (!m_children.isEmpty()) {
        Q_FOREACH(PlaylistItem* child, m_children) {
            child->m_parent = nullptr;
            if (--child->m_useCount == 0)
                delete child;
        }
        m_children.clear();
    }
}

void PlaylistItem::removeAt(int index) {
    auto toRemove = at(index);
    if (!toRemove) return;
    if (index == m_currentIndex) m_currentIndex = -1;
    checkDelete(toRemove);
    m_children.removeAt(index);
}

void PlaylistItem::insert(int index, PlaylistItem *value) {
    if (index >= 0 && index <= count()) {
        value->m_useCount++;
        value->m_parent = this;
        m_children.insert(index, value);
    }
}

int PlaylistItem::indexOf(const QString &link) {
    for (int i = 0; i < m_children.size(); i++) {
        auto child = m_children.at(i);
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
    value->m_useCount++;
    value->m_parent = this;
    m_children.push_back(value);
}

void PlaylistItem::removeOne(PlaylistItem *value) {
    if (!value) return;
    auto index = indexOf(value);
    removeAt(index);

}

void PlaylistItem::reverse() {
    if (m_children.isEmpty()) return;
    std::reverse(m_children.begin(), m_children.end());
}

void PlaylistItem::sort() {
    if (m_children.isEmpty()) return;
    auto currentItem = getCurrentItem();
    std::stable_sort(m_children.begin(), m_children.end(),
                     [](const PlaylistItem *a, const PlaylistItem *b) {
                         if (a->isList() && !b->isList()) return true;
                         if (!a->isList() && b->isList()) return false;
                         if (!a->isList() && !b->isList()) {
                             if (a->seasonNumber == b->seasonNumber) return a->number < b->number;
                             return a->seasonNumber < b->seasonNumber;
                         }
                         return a->name < b->name;
                     });
    if (currentItem)
        setCurrentIndex(indexOf(currentItem));
}


void PlaylistItem::updateHistoryFile() {
    if (!historyFile || !isValidIndex(m_currentIndex)) return;
    if (historyFile->open(QIODevice::WriteOnly)) {
        historyFile->resize(0);
        QTextStream stream(historyFile.get());
        auto item = m_children.at(m_currentIndex);
        QString filename = QFileInfo(item->link).fileName();
        auto timestamp = item->m_timestamp;
        stream << filename;
        if (timestamp > 0) {
            stream << ":" << QString::number(timestamp);
        }
        historyFile->close();
    }
}

void PlaylistItem::checkDelete(PlaylistItem *value) {
    value->m_parent = nullptr;
    if (--value->m_useCount == 0) {
        delete value;
    }
}


bool PlaylistItem::isValidIndex(int index) const {
    if (m_children.isEmpty()) return false;
    return index >= 0 && index < m_children.size();
}
