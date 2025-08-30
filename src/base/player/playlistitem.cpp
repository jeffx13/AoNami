#include "playlistitem.h"
#include "appexception.h"
#include <QtGlobal>
#include <QFileInfo>
#include <cmath>
#include <algorithm>

PlaylistItem::PlaylistItem(int seasonNumber, float number, const QString &link, const QString &name, QSharedPointer<PlaylistItem> parent, bool isLocal)
    : season(seasonNumber), number(number), name(name), link(link), m_parent(parent), type(isLocal ? LOCAL : ONLINE) {
    if (number > -1) {
        bool isInt = floorf(number) == number;
        QString season = seasonNumber != 0 ? QString("S%1").arg(seasonNumber, 2, 10, QChar('0')) : "";
        displayName = season + (isInt ? QString("E%1").arg(int(number), 2, 10, QChar('0')) : QString::number(number, 'f', 1)) ;
        if (!name.isEmpty())
            displayName += QString("\n%1").arg(name);
    } else {
        displayName = name.isEmpty() ? "[Unnamed Episode]" : name;
    }
    
    // Row will be set by parent when added to children list
    m_row = -1;
}

PlaylistItem::~PlaylistItem() {
    clear();
    qDebug() << "Deleted" << (isList() ? "List" : "Item") << (isList() ? link : displayName.simplified() );
}

void PlaylistItem::emplaceBack(int season, float number, const QString &link, const QString &name, bool isLocal) {
    m_children.push_back(QSharedPointer<PlaylistItem>::create(season, number, link, name, sharedFromThis(), isLocal));
    updateRowIndices(m_children.size() - 1);
}

void PlaylistItem::clear() {
    Q_FOREACH (const auto &child, m_children) {
        child->m_parent.clear();
        child->m_row = -1;
    }
    m_children.clear();
}

void PlaylistItem::removeAt(int index) {
    if (!isValidIndex(index)) return;
    if (index == m_currentIndex) m_currentIndex = -1;
    m_children[index]->m_parent.clear();
    m_children.removeAt(index);
    updateRowIndices(index);  // Update row indices for remaining children
}

void PlaylistItem::insert(int index, QSharedPointer<PlaylistItem> value) {
    if (index >= 0 && index <= count()) {
        if (value) value->m_parent = sharedFromThis();
        m_children.insert(index, value);
        updateRowIndices(index);  // Update row indices for inserted and subsequent children
    }
}

int PlaylistItem::indexOf(const QString &link) {
    auto it = std::find_if(m_children.begin(), m_children.end(), 
                          [&link](const QSharedPointer<PlaylistItem>& child) { 
                              return child->link == link; 
                          });
    return it != m_children.end() ? std::distance(m_children.begin(), it) : -1;
}

bool PlaylistItem::setCurrentIndex(int index) {
    if(index != -1 && !isValidIndex(index)) return false;
    m_currentIndex = index;
    return true;
}

void PlaylistItem::append(QSharedPointer<PlaylistItem> value) {
    if (!value) return;
    value->m_parent = sharedFromThis();
    m_children.push_back(value);
    updateRowIndices(m_children.size() - 1);  // Update row index for the appended child
}

void PlaylistItem::removeOne(QSharedPointer<PlaylistItem> value) {
    if (!value) return;
    auto index = indexOf(value);
    removeAt(index);  // removeAt already calls updateRowIndices
}

void PlaylistItem::reverse() {
    if (m_children.isEmpty()) return;
    std::reverse(m_children.begin(), m_children.end());
    updateRowIndices(0);  // Update all row indices after reversing
}

void PlaylistItem::sort() {
    if (m_children.isEmpty()) return;
    auto currentItem = getCurrentItem();
    std::stable_sort(m_children.begin(), m_children.end(),
                     [](const QSharedPointer<PlaylistItem>& a, const QSharedPointer<PlaylistItem>& b) {
                         if (a->isList() && !b->isList()) return true;
                         if (!a->isList() && b->isList()) return false;
                         if (!a->isList() && !b->isList()) {
                             if (a->season == b->season) return a->number < b->number;
                             return a->season < b->season;
                         }
                         return a->name < b->name;
                     });
    if (currentItem)
        setCurrentIndex(indexOf(currentItem));
    updateRowIndices(0);  // Update all row indices after sorting
}

void PlaylistItem::updateHistoryFile() {
    if (!historyFile || !isValidIndex(m_currentIndex)) return;
    if (historyFile->open(QIODevice::WriteOnly)) {
        historyFile->resize(0);
        QTextStream stream(historyFile.data());
        auto item = m_children.at(m_currentIndex);
        QString filename = QFileInfo(item->link).fileName();
        auto timestamp = item->m_timestamp;
        stream << filename;
        if (timestamp > 0)
            stream << ":" << QString::number(timestamp);
        historyFile->close();
    }
}

void PlaylistItem::setTimestamp(qint64 timestamp) {
    if (type == LIST) {
        throw AppException("Cannot set timestamp for list");
    }
    m_timestamp = timestamp;
}

qint64 PlaylistItem::getTimestamp() const {
    if (type == LIST) {
        throw AppException("Cannot get timestamp for list");
    }
    return m_timestamp;
}

bool PlaylistItem::isValidIndex(int index) const {
    if (m_children.isEmpty()) return false;
    return index >= 0 && index < m_children.size();
}

void PlaylistItem::updateRowIndices(int startIndex) {
    for (int i = startIndex; i < m_children.size(); ++i) {
        m_children[i]->m_row = i;
    }
}
