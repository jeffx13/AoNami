#pragma once
#include <QString>
#include <QByteArray>
#include <QMap>
#include <QList>
#include <QUrl>
#include <QMutex>
#include <QNetworkCookie>
#include <QNetworkCookieJar>

// Clears a block by driving the local browser through the challenge. The cf_clearance
// and the UA it was issued to live here; every HTTP path in the app reuses them.
namespace Cloudflare {

constexpr int kBodyScanBytes = 16384;   // how much of a body detection scans

class CookieStore {
public:
    static CookieStore &instance();

    QList<QNetworkCookie> cookiesForUrl(const QUrl &url) const;
    QList<QNetworkCookie> all() const;
    void setCookiesFromUrl(const QList<QNetworkCookie> &cookies, const QUrl &url);

    // Solver output - keeps each cookie's own domain; CF redirects before issuing one.
    void insert(const QList<QNetworkCookie> &cookies);

    // Caller's entries win, unless theirs is an empty placeholder ("__ddg1_=;").
    QByteArray cookieHeader(const QUrl &url, const QString &existing = {}) const;

    void load();
    void save() const;

private:
    CookieStore() = default;

    class Jar : public QNetworkCookieJar {
    public:
        using QNetworkCookieJar::allCookies;
        using QNetworkCookieJar::setAllCookies;
    };

    mutable QMutex m_mutex;
    Jar            m_jar;
    mutable qint64 m_lastSaveMs = 0;
};

class ProxyCookieJar : public QNetworkCookieJar {
public:
    explicit ProxyCookieJar(QObject *parent = nullptr) : QNetworkCookieJar(parent) {}
    QList<QNetworkCookie> cookiesForUrl(const QUrl &url) const override;
    bool setCookiesFromUrl(const QList<QNetworkCookie> &cookies, const QUrl &url) override;
};

bool isBlocked(int code, const QMap<QString, QString> &headers, const QString &body);

void markBlocked(const QString &host);
bool warnOnce(const QString &host, const QString &topic);   // true once per host/topic

QString hostUserAgent(const QString &host);   // walks up labels: pw covers i.pw

// For players with their own HTTP stack (mpv, the HLS proxy).
void applyClearanceHeaders(const QUrl &url, QMap<QString, QString> &headers);
QString takeHeader(QMap<QString, QString> &headers, const char *name);

// Returns the bound UA. Serialised, rate-limited per host.
QString solveChallenge(const QUrl &url, int timeoutMs = 45000);
bool    canSolveChallenge();

} // namespace Cloudflare
