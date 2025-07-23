#include "playlistmanager.h"
#include "utils/logger.h"
#include "utils/myexception.h"
#include "player/mpvObject.h"
#include "utils/errorhandler.h"
#include "providers/showprovider.h"
#include <QtConcurrent/QtConcurrentRun>
// extern "C" {
// #include <ffmpeg/libavformat/avformat.h>
// #include <ffmpeg/libavutil/log.h>
// #include <ffmpeg/libavutil/avutil.h>
// }

PlaylistManager::PlaylistManager(QObject *parent) : QAbstractItemModel(parent)
{
    // Opens the file to play immediately when application launches

    connect (&m_folderWatcher, &QFileSystemWatcher::directoryChanged, this, &PlaylistManager::onLocalDirectoryChanged);
    connect (&m_watcher, &QFutureWatcher<PlayItem>::finished, this, &PlaylistManager::onLoadFinished);
    // about to start
    connect (&m_watcher, &QFutureWatcher<PlayItem>::started, this, [this](){
        m_isCancelled = false;
        setIsLoading(true);
    });

}

void PlaylistManager::onLoadFinished() {
    if (!m_isCancelled.load()) {
        try {
            m_currentPlayItem = m_watcher.result();

            if (!m_currentPlayItem.videos.isEmpty()) {
                // sort videos
                std::sort(m_currentPlayItem.videos.begin(), m_currentPlayItem.videos.end(),
                          [](const Video &a, const Video &b) {
                              if (a.resolution > b.resolution) return true;
                              if (a.resolution < b.resolution) return false;
                              return a.bitrate > b.bitrate;
                          });

                MpvObject::instance()->open(m_currentPlayItem);
                emit aboutToPlay();
            }
        } catch (MyException& ex) {
            ex.show();
        } catch(const std::runtime_error& ex) {
            ErrorHandler::instance().show (ex.what(), "Playlist Error");
        } catch (...) {
            ErrorHandler::instance().show ("Something went wrong", "Playlist Error");
        }
    }
    setIsLoading(false);
    m_isCancelled = false;

    // TODO connect to see if opened successfully as it might lose timestamp from switching servers

}

bool PlaylistManager::tryPlay(int playlistIndex, int itemIndex) {
    if (m_watcher.isRunning()) {
        m_isCancelled = true;
        return false;
    }

    playlistIndex = playlistIndex == -1 ? (m_root->getCurrentIndex() == -1 ? 0 : m_root->getCurrentIndex()) : playlistIndex;
    auto newPlaylist = m_root->at(playlistIndex);
    if (!newPlaylist) return false;

    // Set to current playlist item index if -1
    itemIndex = itemIndex == -1 ? (newPlaylist->getCurrentIndex() == -1 ? 0 : newPlaylist->getCurrentIndex()) : itemIndex;
    if (!newPlaylist->isValidIndex(itemIndex) || newPlaylist->at (itemIndex)->type == PlaylistItem::LIST) {
        oLog() << "Playlist" << "Invalid index or attempting to play a list";
        return false;
    }

    m_isCancelled = false;
    setIsLoading(true);

    m_watcher.setFuture(QtConcurrent::run(&PlaylistManager::play, this, playlistIndex, itemIndex));
    return true;

}

PlayItem PlaylistManager::play(int playlistIndex, int itemIndex) {

    auto playlist = m_root->at(playlistIndex);
    auto episode = playlist->at(itemIndex);

    PlayItem playItem;

    if (episode->type == PlaylistItem::PASTED) {
        if (episode->link.contains('|')) {
            // curl command
            QStringList parts = episode->link.split('|');
            playItem.videos.emplaceBack(parts.takeFirst());
            for (const QString &headerLine : std::as_const(parts)) {
                QStringList keyValue = headerLine.split(": ", Qt::KeepEmptyParts);
                if (keyValue.size() == 2) {
                    playItem.headers.insert(keyValue[0].trimmed(), keyValue[1].trimmed());
                }
            }
        } else {
            playItem.videos.emplaceBack(episode->link);
        }

        m_serverListModel.clear();
    }
    else if (episode->type == PlaylistItem::LOCAL) {
        if (!QDir(playlist->link).exists()) {
            beginResetModel();
            unregisterPlaylist(playlist);
            m_root->removeOne(playlist);
            m_root->setCurrentIndex(m_root->isEmpty() ? -1 : 0);
            endResetModel();
            return playItem;
        }

        if(playlist->getCurrentIndex() != itemIndex){
            playlist->setCurrentIndex(itemIndex);
            playlist->updateHistoryFile();
        }
        playItem.videos.emplaceBack(episode->link);
        m_serverListModel.clear();
    }
    else {
        auto provider = playlist->getProvider();
        if (!provider) throw MyException("Cannot get provider from playlist!", "Provider");

        auto episodeName = episode->getFullName().trimmed().replace('\n', " ");

        // Load server list
        auto servers = provider->loadServers(&m_client, episode);
        if (servers.isEmpty()) throw MyException("No servers found for " + episodeName, "Server");

        // Sort servers by name
        std::sort(servers.begin(), servers.end(), [](const VideoServer &a, const VideoServer &b) {
            return a.name < b.name;
        });

        // Find a working server
        auto result = ServerListModel::findWorkingServer(&m_client, provider, servers);
        if (result.first == -1) throw MyException("No working server found for " + episodeName, "Server");

        if (m_isCancelled.load()) return {};
        // Set the servers and the index of the working server
        m_serverListModel.setServers(servers, provider);
        m_serverListModel.setCurrentIndex(result.first);
        m_serverListModel.setPreferredServer(result.first);
        playItem = result.second;

    }
    if (m_isCancelled.load()) return {};

    playItem.timeStamp = episode->timeStamp;

    m_root->setCurrentIndex(playlistIndex);
    playlist->setCurrentIndex(itemIndex);
    emit currentIndexChanged();
    return playItem;
}

void PlaylistManager::openUrl(QUrl url, bool play) {
    QString urlString = url.toString();

    // if URL is empty, try to get it from clipboard
    if (url.isEmpty()) {
        urlString = QGuiApplication::clipboard()->text().trimmed();

        // check if curl
        static QRegularExpression urlRegex(R"(curl\s+'([^']+)')");
        QRegularExpressionMatch urlMatch = urlRegex.match(urlString);
        if (urlMatch.hasMatch()) {
            QString delimiter = "|";
            QStringList parts;
            parts << urlMatch.captured(1);;
            // Extract headers
            static QRegularExpression headerRegex(R"(-H\s+'([^']+)')");
            QRegularExpressionMatchIterator it = headerRegex.globalMatch(urlString);

            while (it.hasNext()) {
                QRegularExpressionMatch match = it.next();
                QString header = match.captured(1);
                parts << header;
            }
            urlString = parts.join(delimiter);
            url = QUrl::fromUserInput(urlMatch.captured(1));
        }
        else if ((urlString.startsWith('\'') && urlString.endsWith('\'')) ||
                   (urlString.startsWith('"') && urlString.endsWith('"')))
        {
            urlString.removeAt(0);
            urlString.removeLast();
            urlString.replace("\\/", "/");
            url = QUrl::fromUserInput(urlString);
        }


    }

    if (!url.isValid()) return;

    static QStringList m_subtitleExtensions = { "srt", "sub", "ssa", "ass", "idx", "vtt" };
    if (m_subtitleExtensions.contains(QFileInfo(url.path()).suffix()) || url.path().toLower().contains("subtitle") ) {
        // int subtitleIndex = m_subtitleListModel.addSubtitle(url);
        MpvObject::instance()->addSubtitle(Track(url));
        // loadSubtitle(subtitleIndex);
        return;
    }

    // static QRegularExpression urlPattern(R"(https?:\/\/(?:www\.|(?!www))[a-zA-Z0-9][a-zA-Z0-9-]+[a-zA-Z0-9]\.[^\s]{2,}|www\.[a-zA-Z0-9][a-zA-Z0-9-]+[a-zA-Z0-9]\.[^\s]{2,}|https?:\/\/(?:www\.|(?!www))[a-zA-Z0-9]+\.[^\s]{2,}|www\.[a-zA-Z0-9]+\.[^\s]{2,})");
    int playlistIndex = -1;
    if (url.isLocalFile()) {
        cLog() << "Playlist" << "Opening local file" << url;
        playlistIndex = append(PlaylistItem::fromLocalUrl(url));
    } else { // Online video
        cLog() << "Playlist" << "Opening online video" << urlString;

        playlistIndex = m_root->indexOf("videos");
        if (playlistIndex == -1) {
            // Create a playlist for pasted videos
            playlistIndex = append(new PlaylistItem("Videos", nullptr, "videos"));
        }

        PlaylistItem *pastePlaylist = m_root->at(playlistIndex);
        auto itemIndex = pastePlaylist->indexOf(urlString);
        if (itemIndex == -1) {
            auto parent = createIndex(playlistIndex, 0, pastePlaylist);
            beginInsertRows(parent, pastePlaylist->size(), pastePlaylist->size());
            pastePlaylist->emplaceBack(0, pastePlaylist->size() + 1, urlString, urlString, true);
            pastePlaylist->last();

            pastePlaylist->last()->type = PlaylistItem::PASTED;
            itemIndex = pastePlaylist->size() - 1;
        }
        pastePlaylist->setCurrentIndex(itemIndex);
    }

    if (play && playlistIndex != -1 && MpvObject::instance()->getCurrentVideoUrl() != url) {
        MpvObject::instance()->showText(QString("Playing: %1").arg(urlString.toUtf8()));
        tryPlay(playlistIndex);
    }

}

void PlaylistManager::onLocalDirectoryChanged(const QString &path) {
    int index = m_root->indexOf(path);
    if (index == -1)  return;
    auto playlist = m_root->at(index);

    QString prevlink;
    bool isCurrent = m_root->getCurrentIndex() == index && playlist->getCurrentIndex() != -1;
    if (isCurrent) {
        prevlink = playlist->getCurrentItem()->link;
    }

    if (!playlist->reloadFromFolder()) {
        // Folder is empty, deleted, can't open history file etc.
        unregisterPlaylist(playlist);
        beginResetModel();
        m_root->removeAt(index);
        endResetModel();
        m_root->setCurrentIndex(m_root->isEmpty() ? -1 : 0);
        emit currentIndexChanged();
        cLog() << "Playlist" << "Failed to reload folder" << m_root->at(index)->link;
    }

    if (isCurrent) {
        QString newLink = playlist->getCurrentItem()->link;
        if (prevlink != newLink) {
            tryPlay();
        }
    }
}

// bool PlaylistManager::parseLocalVideo(PlayItem &playItem) {
//     AVFormatContext *fmtCtx = nullptr;
//     auto filePath = playItem.localFile.toString().toStdString();
//     // Open the input file (media file)
//     if (avformat_open_input(&fmtCtx, filePath.c_str(), nullptr, nullptr) < 0) {
//         return false;
//     }
//     // Retrieve stream information
//     if (avformat_find_stream_info(fmtCtx, nullptr) < 0) {
//         avformat_close_input(&fmtCtx);
//         return false;
//     }
//     // const char *formatName = fmtCtx->iformat->name;
//     // gLog() << "Playlist" << "File format:" << formatName;

//     // Iterate through streams and print information about each stream
//     for (unsigned int i = 0; i < fmtCtx->nb_streams; i++) {
//         AVStream *stream = fmtCtx->streams[i];
//         AVCodecParameters *codecpar = stream->codecpar;

//         switch (codecpar->codec_type) {
//         case AVMEDIA_TYPE_VIDEO: {
//             // gLog() << "Playlist" << "Video stream found!";
//             // gLog() << "Codec: " << avcodec_get_name(codecpar->codec_id);
//             // gLog() << "Width: " << codecpar->width;
//             // gLog() << "Height: " << codecpar->height;
//             // gLog() << "Bitrate: " << codecpar->bit_rate;
//             // gLog() << "Frame rate: " << av_q2d(stream->r_frame_rate);
//             // gLog() << "Sample aspect ratio: " << stream->sample_aspect_ratio.num << "/" << stream->sample_aspect_ratio.den;
//             // gLog() << "Metadata: ";
//             int height = codecpar->height;
//             QString title = "";
//             if (stream->metadata) {
//                 // AVDictionaryEntry *tag = nullptr;
//                 // while ((tag = av_dict_get(stream->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
//                 //     gLog() << QString(tag->key) << QString(tag->value);
//                 // }
//                 AVDictionaryEntry *titleTag = av_dict_get(stream->metadata, "title", nullptr, 0);
//                 if (titleTag) {
//                     title = QString(titleTag->value);
//                 }
//             }
//             playItem.videos.emplaceBack(QUrl(), title, height);

//             break;
//         }
//         case AVMEDIA_TYPE_AUDIO:{
//             // gLog() << "Playlist" << "Audio stream found!";
//             // gLog() << "Codec: " << avcodec_get_name(codecpar->codec_id);
//             // gLog() << "Sample rate" << codecpar->sample_rate;
//             // gLog() << "Bitrate" << codecpar->bit_rate;
//             // gLog() << "Sample format" << av_get_sample_fmt_name((AVSampleFormat)codecpar->format);
//             // gLog() << "Metadata";
//             // Print metadata if available
//             QString label;
//             if (stream->metadata) {
//                 AVDictionaryEntry *titleTag = av_dict_get(stream->metadata, "title", nullptr, 0);
//                 AVDictionaryEntry *languageTag = av_dict_get(stream->metadata, "language", nullptr, 0);
//                 label = titleTag ? QString(titleTag->value) : QString();
//                 if (languageTag) {
//                     label = label.isEmpty() ? QString(languageTag->value) : label + "\n[" + QString(languageTag->value) + "]";
//                 }
//             }
//             playItem.audios.emplaceBack(QUrl(), label);
//         }
//         break;

//         case AVMEDIA_TYPE_SUBTITLE:{
//             // gLog() << "Playlist" << "Subtitle stream found!";
//             // gLog() << "Codec" << avcodec_get_name(codecpar->codec_id);
//             // gLog() << "Metadata";
//             // Print metadata if available
//             QString label;
//             if (stream->metadata) {
//                 AVDictionaryEntry *titleTag = av_dict_get(stream->metadata, "title", nullptr, 0);
//                 AVDictionaryEntry *languageTag = av_dict_get(stream->metadata, "language", nullptr, 0);
//                 label = titleTag ? QString(titleTag->value) : QString();
//                 if (languageTag) {
//                     label = label.isEmpty() ? QString(languageTag->value) : label + "\n[" + QString(languageTag->value) + "]";
//                 }
//             }
//             playItem.subtitles.emplaceBack(QUrl(), label);
//             break;
//         }
//         case AVMEDIA_TYPE_ATTACHMENT:
//             // gLog() << "Playlist" << "Attachment stream found!";
//             // gLog() << "Codec: " << avcodec_get_name(codecpar->codec_id);
//             // gLog() << "Filename: " << codecpar->codec_tag;
//             // gLog() << "Metadata: ";
//             // Print metadata if available
//             // if (stream->metadata) {
//             //     AVDictionaryEntry *tag = nullptr;
//             //     while ((tag = av_dict_get(stream->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
//             //         gLog() << QString(tag->key) << QString(tag->value);
//             //     }
//             // }
//             break;
//         default:
//             // gLog() << "Playlist" << "Other stream type found!";
//             // gLog() << "Codec: " << avcodec_get_name(codecpar->codec_id);
//             break;
//         }
//     }

//     // Close the format context after use
//     avformat_close_input(&fmtCtx);

//     return true;
// }

void PlaylistManager::loadServer(int index) {
    if (m_watcher.isRunning()) {
        m_isCancelled = true;
        return;
    }
    if (!m_serverListModel.isValidIndex(index)) return;

    m_watcher.setFuture(QtConcurrent::run([&, index](){
        auto client = Client(&m_isCancelled);
        auto serverName = m_serverListModel.at(index).name;
        PlayItem playItem = m_serverListModel.loadServer(&client, index);
        if (playItem.videos.isEmpty()) {
            // throw MyException(QString("Failed to load server %1").arg(serverName), "Server");
            oLog() << "Server" << QString("Failed to load server %1").arg(serverName);
            return playItem;
        }
        playItem.timeStamp = MpvObject::instance()->time();
        m_serverListModel.setCurrentIndex(index);
        m_serverListModel.setPreferredServer(index);

        return playItem;
    }));
}

void PlaylistManager::loadVideo(int index) {
    MpvObject::instance()->setVideoIndex(index);
}

void PlaylistManager::loadAudio(int index) {
    MpvObject::instance()->setAudioIndex(index);
}

void PlaylistManager::loadSubtitle(int index) {
    MpvObject::instance()->setSubIndex(index);
}

void PlaylistManager::loadIndex(QModelIndex index) {
    auto childItem = static_cast<PlaylistItem *>(index.internalPointer());
    auto parentItem = childItem->parent();
    if (parentItem == m_root) return;
    int itemIndex = childItem->row();
    int playlistIndex = m_root->indexOf(parentItem);

    tryPlay(playlistIndex, itemIndex);
}

void PlaylistManager::loadOffset(int offset) {
    auto currentPlaylist = m_root->getCurrentItem();
    if (!currentPlaylist) return;
    auto currentIndex = currentPlaylist->getCurrentIndex();
    int newIndex = currentIndex + offset;
    // Reached playlist end so start playing next playlist
    if (newIndex == currentPlaylist->size() && m_root->getCurrentIndex() + 1 < m_root->size()) {
        auto nextPlaylist = m_root->at(m_root->getCurrentIndex() + 1);
        newIndex = nextPlaylist->getCurrentIndex() == -1 ? 0 : nextPlaylist->getCurrentIndex();
        currentPlaylist = nextPlaylist;
        m_root->setCurrentIndex(m_root->getCurrentIndex() + 1);
    } else if (newIndex < 0 && m_root->getCurrentIndex() - 1 >= 0) {
        // Reached playlist start so start playing previous playlist
        auto prevPlaylist = m_root->at(m_root->getCurrentIndex() - 1);
        newIndex = prevPlaylist->getCurrentIndex() == -1 ? prevPlaylist->size() - 1 : prevPlaylist->getCurrentIndex();
        currentPlaylist = prevPlaylist;
        m_root->setCurrentIndex(m_root->getCurrentIndex() - 1);
    }

    if (!currentPlaylist->isValidIndex(newIndex)) return;

    tryPlay(-1, newIndex);
}

void PlaylistManager::reload() {
    auto currentPlaylist = m_root->getCurrentItem();
    if (!currentPlaylist || !currentPlaylist->getCurrentItem()) return;
    auto time = MpvObject::instance()->time();
    currentPlaylist->getCurrentItem()->timeStamp = time;
    tryPlay();
}

bool PlaylistManager::registerPlaylist(PlaylistItem *playlist) {
    if (!playlist || playlistSet.contains(playlist->link)) return false;
    playlist->use();
    playlistSet.insert(playlist->link);

    // Watch playlist path if local folder
    if (playlist->isLoadedFromFolder()) {
        m_folderWatcher.addPath(playlist->link);
    }
    return true;
}

void PlaylistManager::unregisterPlaylist(PlaylistItem *playlist) {
    if (!playlist || !playlistSet.contains(playlist->link)) return;
    playlistSet.remove(playlist->link);
    // Unwatch playlist path if local folder
    if (playlist->isLoadedFromFolder()) {
        m_folderWatcher.removePath(playlist->link);
    }

}

int PlaylistManager::append(PlaylistItem *playlist) {
    if (!playlist) return -1;
    if (!registerPlaylist(playlist)) {
        return m_root->indexOf(playlist->link);
    }
    auto row = m_root->size();
    beginInsertRows(QModelIndex(), row, row);
    m_root->append(playlist);
    endInsertRows();
    return row;
}

int PlaylistManager::insert(int index, PlaylistItem *playlist) {
    if (index < 0 || index >= m_root->size()) {
        return append(playlist);
    }
    if (!registerPlaylist(playlist)) {
        return m_root->indexOf(playlist->link);
    }
    beginInsertRows(QModelIndex(), index, index);
    m_root->insert(index, playlist);
    endInsertRows();
    return index;
}

int PlaylistManager::replace(int index, PlaylistItem *newPlaylist) {
    if (m_root->isEmpty() || index < 0 || index >= m_root->size()) {
        return append(newPlaylist);
    }
    if (!registerPlaylist(newPlaylist)) {
        return m_root->indexOf(newPlaylist->link);
    }
    auto playlistToReplace = m_root->at(index);
    if (!newPlaylist || !playlistToReplace) return -1;

    unregisterPlaylist(playlistToReplace);
    beginRemoveRows(QModelIndex(), index, index);
    m_root->removeAt(index);
    endRemoveRows();
    beginInsertRows(QModelIndex(), index, index);
    m_root->insert(index, newPlaylist);
    endInsertRows();
    return index;
}

void PlaylistManager::removeAt(int index) {
    // Validate index and ensure we're not removing the currently playing playlist
    if (!m_root->isValidIndex(index) || index == m_root->getCurrentIndex()) {
        return;
    }

    PlaylistItem *playlistToRemove = m_root->at(index);
    if (!playlistToRemove) return;

    // Storing currentPlaylist before removal
    PlaylistItem *currentPlaylist = m_root->getCurrentItem();

    // Begin removal operation
    beginRemoveRows(QModelIndex(), index, index);
    unregisterPlaylist(playlistToRemove);
    m_root->removeAt(index);
    endRemoveRows();

    // currentPlaylist is still be valid, but its index might have changed.
    if (currentPlaylist) {
        int newCurrentIndex = m_root->indexOf(currentPlaylist);
        m_root->setCurrentIndex(newCurrentIndex);
        emit currentIndexChanged();
    }
}

void PlaylistManager::clear() {
    beginRemoveRows(QModelIndex(), 0, m_root->size() - 1);
    auto currentPlaylist = m_root->getCurrentItem();
    for (int i = 0; i < m_root->size(); i++) {
        const auto &playlist = m_root->at(i);
        if (playlist == currentPlaylist) continue;
        unregisterPlaylist(playlist);
        m_root->removeOne(playlist);
    }
    endRemoveRows();
    beginInsertRows(QModelIndex(), 0, 0);
    endInsertRows();
    m_root->setCurrentIndex(m_root->isEmpty() ? -1 : 0);
    emit currentIndexChanged();

}

void PlaylistManager::setIsLoading(bool value) {
    m_isLoading = value;
    emit isLoadingChanged();
}

TrackListModel *PlaylistManager::getVideoList() { return MpvObject::instance()->getVideoList(); }

TrackListModel *PlaylistManager::getAudioList() { return MpvObject::instance()->getAudioList(); }

TrackListModel *PlaylistManager::getSubtitleList() { return MpvObject::instance()->getSubtitleList(); }

void PlaylistManager::showCurrentItemName() const {
    auto currentPlaylist = m_root->getCurrentItem();
    if (!currentPlaylist) return;
    auto itemName = currentPlaylist->getDisplayNameAt(currentPlaylist->getCurrentIndex());
    MpvObject::instance()->showText(itemName);
}

QModelIndex PlaylistManager::getCurrentIndex(QModelIndex i) const {
    auto currentPlaylist = static_cast<PlaylistItem *>(i.internalPointer());
    if (!currentPlaylist ||
        !currentPlaylist->isValidIndex(currentPlaylist->getCurrentIndex()))
        return QModelIndex();
    return index(currentPlaylist->getCurrentIndex(), 0, index(m_root->indexOf(currentPlaylist), 0, QModelIndex()));
}

QModelIndex PlaylistManager::getCurrentModelIndex() const {
    PlaylistItem *currentPlaylist = m_root->getCurrentItem();
    if (!currentPlaylist ||
        !currentPlaylist->isValidIndex(currentPlaylist->getCurrentIndex()))
        return QModelIndex();

    return index(currentPlaylist->getCurrentIndex(), 0, index(m_root->getCurrentIndex(), 0, QModelIndex()));
}

QModelIndex PlaylistManager::getCurrentListIndex() {
    return createIndex(m_root->getCurrentIndex(), 0, m_root->getCurrentItem());
}

int PlaylistManager::rowCount(const QModelIndex &parent) const {
    if (parent.column() > 0) return 0;

    const PlaylistItem *parentItem = parent.isValid() ? static_cast<PlaylistItem*>(parent.internalPointer()) : m_root;
    return parentItem->size();
}

QModelIndex PlaylistManager::index(int row, int column, const QModelIndex &parent) const {
    if (!hasIndex(row, column, parent)) return QModelIndex();
    PlaylistItem *parentItem = parent.isValid() ? static_cast<PlaylistItem *>(parent.internalPointer()) : m_root;
    PlaylistItem *childItem = parentItem->at(row);
    return childItem ? createIndex(row, column, childItem) : QModelIndex();
}

QModelIndex PlaylistManager::parent(const QModelIndex &childIndex) const {
    if (!childIndex.isValid()) return QModelIndex();

    PlaylistItem *childItem = static_cast<PlaylistItem *>(childIndex.internalPointer());
    PlaylistItem *parentItem = childItem->parent();

    if (parentItem == m_root || parentItem == nullptr) return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

QVariant PlaylistManager::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || m_root->isEmpty())
        return QVariant();
    auto item = static_cast<PlaylistItem*>(index.internalPointer());

    switch (role) {
    case TitleRole:
        return item->name;
        break;
    case IndexRole:
        return index;
        break;
    case NumberRole:
        return item->number;
        break;
    case NumberTitleRole: {
        if (item->type == 0) {
            return item->name;
        }
        return item->getFullName();
        break;
    case IsCurrentIndexRole:
        if (!item->parent() || item->parent()->getCurrentIndex() == -1) return false;
        return item->parent()->getCurrentItem() == item;
        break;
    }
    default:
        break;
    }
    return QVariant();
}

QHash<int, QByteArray> PlaylistManager::roleNames() const {
    QHash<int, QByteArray> names
        {
            {TitleRole, "title"},
            {NumberRole, "number"},
            {IndexRole, "index"},
            {NumberTitleRole, "numberTitle"},
            {IsCurrentIndexRole, "isCurrentIndex"}
        };
    return names;
}

