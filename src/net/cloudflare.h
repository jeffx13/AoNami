#pragma once
#include <QString>
#include <QByteArray>
#include <QMap>
#include <QList>
#include <QUrl>
#include <QMutex>
#include <QNetworkCookie>
#include <QNetworkCookieJar>
#include "net/canceltoken.h"

// Clears a block by driving the local browser. The cf_clearance and the UA it was
// issued to live here - every HTTP path in the app reuses them.
namespace Cloudflare {

constexpr int kBodyScanBytes = 16384;   // how much of a body detection scans

class CookieStore {
public:
    static CookieStore &instance();

    QList<QNetworkCookie> cookiesForUrl(const QUrl &url) const;
    void setCookiesFromUrl(const QList<QNetworkCookie> &cookies, const QUrl &url);

    // Solver output - keeps each cookie's own domain; CF redirects before issuing one.
    void insert(const QList<QNetworkCookie> &cookies);

    // Caller's entries win, unless theirs is an empty placeholder ("__ddg1_=;").
    QByteArray cookieHeader(const QUrl &url, const QString &existing = {}) const;

    void load();
    void save() const;
    void flush() const;   // write out a save the rate limiter deferred

private:
    CookieStore() = default;

    class Jar : public QNetworkCookieJar {
    public:
        using QNetworkCookieJar::allCookies;
        using QNetworkCookieJar::setAllCookies;
    };

    mutable QMutex m_mutex;
    Jar            m_jar;
    mutable qint64 m_lastSaveMs  = 0;
    mutable bool   m_pendingSave = false;
};

class ProxyCookieJar : public QNetworkCookieJar {
public:
    explicit ProxyCookieJar(QObject *parent = nullptr) : QNetworkCookieJar(parent) {}
    QList<QNetworkCookie> cookiesForUrl(const QUrl &url) const override;
    bool setCookiesFromUrl(const QList<QNetworkCookie> &cookies, const QUrl &url) override;
};

bool isBlocked(int code, const QMap<QString, QString> &headers, const QString &body);

void markBlocked(const QString &host);

QString hostUserAgent(const QString &host);   // walks up labels: pw covers i.pw

void applyClearanceHeaders(const QUrl &url, QMap<QString, QString> &headers);

// Returns the bound UA. Serialised and rate-limited per host.
QString solveChallenge(const QUrl &url, const CancelToken &cancel = {}, int timeoutMs = 45000);
bool    canSolveChallenge();

// Abandons any solve in flight - a closing app would otherwise sit behind one.
void shutdown();

} // namespace Cloudflare
