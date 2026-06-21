#pragma once

// Compatibility shim: defines __MINGW_CXX11/14_CONSTEXPR (used by MSYS2's winnt.h)
// that Qt's MinGW lacks. Must be included before <windows.h>.

#ifdef _WIN32

#ifndef __MINGW_CXX11_CONSTEXPR
#  ifdef __cpp_constexpr
#    define __MINGW_CXX11_CONSTEXPR constexpr
#  else
#    define __MINGW_CXX11_CONSTEXPR
#  endif
#endif

#ifndef __MINGW_CXX14_CONSTEXPR
#  if __cpp_constexpr >= 201304L
#    define __MINGW_CXX14_CONSTEXPR constexpr
#  else
#    define __MINGW_CXX14_CONSTEXPR
#  endif
#endif

#endif // _WIN32

// Prefer specific module headers over blanket <QtCore>/<QtGui> etc.
// The module-level includes pull in hundreds of headers and slow PCH generation.
#include <QObject>
#include <QString>
#include <QByteArray>
#include <QList>
#include <QMap>
#include <QHash>
#include <QSet>
#include <QVariant>
#include <QUrl>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTimer>
#include <QDateTime>
#include <QRegularExpression>
#include <QSharedPointer>
#include <QWeakPointer>
#include <QScopedPointer>
#include <QDebug>
#include <QCoreApplication>

#include <QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>
#include <QGuiApplication>
#include <QQmlEngine>

#include <atomic>
#include <memory>
#include <functional>
#include <utility>
#include <algorithm>
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <optional>
