#pragma once

// Qt
class QString;
class QByteArray;
class QVariant;
class QModelIndex;
class QUrl;
class QRegularExpression;

template <typename T> class QList;
template <typename Key, typename T> class QMap;
template <typename Key, typename T> class QHash;
template <typename T> class QSharedPointer;
template <typename T> class QWeakPointer;
template <typename T> class QFutureWatcher;
template <typename T> class QFuture;

// Base
class ShowProvider;
struct ShowData;
struct PlayInfo;
struct VideoServer;
struct Track;
struct Video;
class PlaylistItem;
class PlaylistManager;

// Managers
class ProviderManager;
class SearchManager;
class ShowManager;
class LibraryManager;
class DownloadManager;

// Models
class LibraryProxyModel;
class ServerListModel;
class TrackListModel;
class EpisodeListModel;
class LogListModel;

// App
class Settings;
class Application;
