#include "playlistitem.h"




bool PlaylistItem::loadFromFolder(const QUrl &pathUrl) {
    if (!m_isLoadedFromFolder) return false;
    clear();

    QDir playlistDir;
    QString lastPlayedFilename = "";
    QString openedFilename = "";
    QString timeString = "";

    if (!pathUrl.isEmpty()) {
        QFileInfo path = QFileInfo(pathUrl.toLocalFile());
        if (!path.exists()) {
            qDebug() << "Log (Playlist)   : Path" << path << "doesn't exist";
            return false;
        }

        if (!path.isDir()) {
            playlistDir = path.dir();
            openedFilename = path.fileName();
        } else {
            playlistDir = QDir(pathUrl.toLocalFile());
        }
        //qDebug() << playlistDir <<playlistDir.dirName() << playlistDir.absolutePath();
        m_historyFile = std::make_unique<QFile> (playlistDir.filePath(".mpv.history"));
        name = playlistDir.dirName();
        link = playlistDir.absolutePath();

    } else {
        if (link.isEmpty()) return false;
        playlistDir = QDir(link);
        if (!playlistDir.exists()) {
            qDebug() << "Log (Playlist)   : Path" << link << "doesn't exist";
            currentIndex = -1;
            return false;
        }
    }


    QStringList fileNames = playlistDir.entryList(
        {"*.mp4", "*.mkv", "*.avi", "*.mp3", "*.flac", "*.wav", "*.ogg", "*.webm", "*.m3u8"}, QDir::Files);
    if (fileNames.isEmpty()) {
        qDebug() << "Log (Playlist)   : No files to play in" << playlistDir.absolutePath();
        currentIndex = -1;
        return false;
    }

    // Read history file
    if (m_historyFile->exists()) {
        // Open history file
        bool fileOpened = m_historyFile->open(QIODevice::ReadOnly | QIODevice::Text);
        if (!fileOpened) {
            qDebug() << "Log (Playlist)   : Failed to open history file";
            currentIndex = -1;
            return false;
        }
        auto fileData = QTextStream(m_historyFile.get()).readAll().trimmed().split(":");
        m_historyFile->close();
        if (!fileData.isEmpty()) {
            lastPlayedFilename = fileData.first();
            if (fileData.size() == 2) {
                timeString = fileData.last();
            }
        }
    }

    // Check if the opened file is different from the last played file
    if (!openedFilename.isEmpty() && lastPlayedFilename != openedFilename) {
        bool fileOpened = m_historyFile->open(QIODevice::WriteOnly | QIODevice::Text);
        if (!fileOpened) {
            qDebug() << "Log (Playlist)   : Failed to open history file";
            return false;
        }
        m_historyFile->write(openedFilename.toUtf8());
        m_historyFile->close();
        timeString.clear();
        lastPlayedFilename = openedFilename;
    }

    PlaylistItem *currentItemPtr = nullptr;

    for (int i = 0; i < fileNames.count(); i++) {
        QRegularExpressionMatch match = fileNameRegex.match(fileNames[i]);
        QString title = match.hasMatch() ? match.captured("title").trimmed() : "";
        int itemNumber = match.hasMatch() ? !match.captured("number").isEmpty() ? match.captured("number").toInt() : i : i;
        emplaceBack (0, itemNumber,  playlistDir.absoluteFilePath(fileNames[i]), title, true);
        if (fileNames[i] == lastPlayedFilename) {
            // Set current item
            // qDebug() << fileNames[i];
            currentItemPtr = m_children->last();
        }
    }

    // sort the episodes in order
    std::stable_sort(m_children->begin(), m_children->end(),
                     [](const PlaylistItem *a, const PlaylistItem *b) {
                         return a->number < b->number;
                     });

    if (currentItemPtr) {
        currentIndex = indexOf (currentItemPtr);
        if (!timeString.isEmpty()) {
            bool ok;
            int intTime = timeString.toInt (&ok);
            if (ok) {
                currentItemPtr->timeStamp = intTime;
            }
        }
    }

    if (currentIndex < 0) currentIndex = 0;
    return true;
}

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
    if (parent) useCount++;

}

PlaylistItem *PlaylistItem::fromLocalUrl(const QUrl &pathUrl) {

    if (!pathUrl.isLocalFile())
        return nullptr;
    // auto pathString = pathUrl.toString();
    // PlaylistItem *playlist = new PlaylistItem("Pasted Link", nullptr, "pasted");
    // playlist->emplaceBack (-1, pathString, pathString, true);
    // playlist->currentIndex = 0;
    // return playlist;

    PlaylistItem *playlist = new PlaylistItem("", nullptr, "");
    playlist->m_isLoadedFromFolder = true;
    playlist->m_children = std::make_unique<QList<PlaylistItem*>>();
    if (!playlist->loadFromFolder (pathUrl)) {
        delete playlist;
        return nullptr;
    }

    return playlist;
}

void PlaylistItem::emplaceBack(int seasonNumber, float number, const QString &link, const QString &name, bool isLocal) {
    if (!m_children) m_children = std::make_unique<QList<PlaylistItem*>>();
    auto playlistItem = new PlaylistItem(seasonNumber, number, link, name, this, isLocal);
    m_children->push_back(playlistItem);
}

void PlaylistItem::clear() {
    if (m_children) {
        for (auto &playlist : *m_children) {
            if (--playlist->useCount == 0)
                delete playlist;
        }
        m_children->clear();
    }
}

void PlaylistItem::removeAt(int index) {
    auto toRemove = at(index);
    if (!m_children || !toRemove) return;
    checkDelete (toRemove);
    m_children->removeAt (index);
}

bool PlaylistItem::replace(int index, PlaylistItem *value) {
    auto toRemove = at(index);
    if (!toRemove) return false;
    toRemove->m_parent = nullptr;
    if (--toRemove->useCount == 0) {
        delete toRemove;
    }
    m_children->replace(index, value);
    value->m_parent = this;
    value->useCount++;
    return true;
}

int PlaylistItem::indexOf(const QString &link) {
    if (!m_children) return -1;
    for (int i = 0; i < m_children->size(); i++) {
        auto child = m_children->at (i);
        if (child->link == link) {
            return i;
        }
    }
    return -1;
}

void PlaylistItem::append(PlaylistItem *value) {
    if (!m_children) m_children = std::make_unique<QList<PlaylistItem*>>();
    value->useCount++;
    value->m_parent = this;
    m_children->push_back (value);
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

void PlaylistItem::updateHistoryFile(qint64 time) {
    if (!m_isLoadedFromFolder) return;
    static QMutex mutex;
    mutex.lock();
    if (m_historyFile->isOpen() || m_historyFile->open(QIODevice::WriteOnly)) {
        m_historyFile->resize(0);
        QTextStream stream(m_historyFile.get());
        QString lastWatchedFilePath = m_children->at(currentIndex)->link;
        stream << lastWatchedFilePath.split("/").last();
        if (time > 0) {
            stream << ":" << QString::number (time);
        }
        m_historyFile->close();
    }
    mutex.unlock();
}

void PlaylistItem::setLastPlayAt(int index, int time) {
    if (!isValidIndex (index)) return;
    qDebug() << "Log (Playlist)   ： Setting playlist last play info at" << index << time;
    currentIndex = index;
    m_children->at (index)->timeStamp = time;
}

bool PlaylistItem::isValidIndex(int index) const {
    if (!m_children || m_children->isEmpty()) return false;
    return index >= 0 && index < m_children->size();
}
