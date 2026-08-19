#include "media/playlistitem.h"
#include <QtGlobal>
#include <QFileInfo>
#include <cmath>
#include <algorithm>

PlaylistItem::PlaylistItem(const QString& name, ShowProvider* provider, const QString &link)
    : name(name), m_provider(provider), link(link), type(LIST) {}

PlaylistItem::PlaylistItem(int seasonNumber, float number, const QString &link, const QString &name,
                           QSharedPointer<PlaylistItem> parent, bool isLocal, bool preview)
    : season(seasonNumber), number(number), name(name), link(link), m_parent(parent),
    type(isLocal ? LOCAL : ONLINE), preview(preview)
{
    if (number > -1) {
        bool isInt = floorf(number) == number;
        QString seasonStr = seasonNumber != 0 ? QString("S%1").arg(seasonNumber, 2, 10, QChar('0')) : "";
        displayName = seasonStr
                      + (isInt ? QString("E%1").arg(int(number), 2, 10, QChar('0'))
                               : QString::number(number, 'f', 1));
        if (!name.isEmpty())
            displayName += QString("\n%1").arg(name);
    } else {
        displayName = name.isEmpty() ? "[Unnamed Episode]" : name;
    }
    m_row = -1;
}

PlaylistItem::~PlaylistItem() {
    clear();
}

void PlaylistItem::emplaceBack(int season, float number, const QString &link, const QString &name, bool isLocal, bool preview) {
    m_children.push_back(QSharedPointer<PlaylistItem>::create(season, number, link, name, sharedFromThis(), isLocal, preview));
    updateRowIndices(m_children.size() - 1);
}

void PlaylistItem::append(QSharedPointer<PlaylistItem> value) {
    if (!value) return;
    value->m_parent = sharedFromThis();
    m_children.push_back(value);
    updateRowIndices(m_children.size() - 1);
}

void PlaylistItem::insert(int index, QSharedPointer<PlaylistItem> value) {
    if (!value) return;
    if (index >= 0 && index <= count()) {
        value->m_parent = sharedFromThis();
        m_children.insert(index, value);
        updateRowIndices(index);
    }
}

void PlaylistItem::removeAt(int index) {
    if (!isValidIndex(index)) return;
    if (index == m_currentIndex)       m_currentIndex = -1;
    else if (index < m_currentIndex)   m_currentIndex--;   // keep pointing at the same item
    m_children[index]->m_parent.clear();
    m_children.removeAt(index);
    updateRowIndices(index);
}

void PlaylistItem::removeOne(QSharedPointer<PlaylistItem> value) {
    if (!value) return;
    auto index = indexOf(value);
    removeAt(index);
}

void PlaylistItem::clear() {
    for (const auto &child : std::as_const(m_children)) {
        child->m_parent.clear();
        child->m_row = -1;
    }
    m_children.clear();
}

void PlaylistItem::sort() {
    if (m_children.isEmpty()) return;
    auto currentItem = getCurrentItem();
    std::stable_sort(m_children.begin(), m_children.end(),
                     [](const QSharedPointer<PlaylistItem>& a, const QSharedPointer<PlaylistItem>& b) {
                         // Lists before items
                         if (a->isList() && !b->isList()) return true;
                         if (!a->isList() && b->isList()) return false;
                         // Items: sort by season then episode number
                         if (!a->isList() && !b->isList()) {
                             if (a->season == b->season) return a->number < b->number;
                             return a->season < b->season;
                         }
                         // Lists: sort by name
                         return a->name < b->name;
                     });
    if (currentItem)
        setCurrentIndex(indexOf(currentItem));
    updateRowIndices(0);
}

int PlaylistItem::indexOf(const QString &link) {
    auto it = std::find_if(m_children.begin(), m_children.end(),
                           [&link](const QSharedPointer<PlaylistItem>& child) {
                               return child->link == link;
                           });
    return it != m_children.end() ? std::distance(m_children.begin(), it) : -1;
}

bool PlaylistItem::isValidIndex(int index) const {
    if (m_children.isEmpty()) return false;
    return index >= 0 && index < m_children.size();
}

bool PlaylistItem::setCurrentIndex(int index) {
    if (index != -1 && !isValidIndex(index)) return false;
    m_currentIndex = index;
    return true;
}

void PlaylistItem::setTimestamp(qint64 timestamp) {
    if (type & LIST) {
        Q_ASSERT(false);
        return;
    }
    m_timestamp = timestamp;
}

qint64 PlaylistItem::getTimestamp() const {
    if (type & LIST) {
        Q_ASSERT(false);
        return 0;
    }
    return m_timestamp;
}

void PlaylistItem::updateHistoryFile() {
    if (!historyFile || !isValidIndex(m_currentIndex)) return;
    if (historyFile->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
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

void PlaylistItem::updateRowIndices(int startIndex) {
    for (int i = startIndex; i < m_children.size(); ++i) {
        m_children[i]->m_row = i;
    }
}
