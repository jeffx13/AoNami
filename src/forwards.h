#pragma once

// Centralized forward declarations to reduce header dependencies

// Qt forward declarations
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

// Project forward declarations
// Base
class ServiceManager;
class ShowProvider;
class ShowData;

// Player/Playlist
class PlaylistItem;
class PlaylistManager;

// Managers
class ProviderManager;
class SearchManager;
class ShowManager;
class LibraryManager;
class DownloadManager;

// Models
class DownloadListModel;
class LibraryModel;
class LibraryProxyModel;
class PlaylistModel;
class SearchResultModel;
class ServerListModel;

// App-level
class Settings;
class LogListModel;


