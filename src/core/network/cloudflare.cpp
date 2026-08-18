#include "cloudflare.h"
#include "network.h"
#include "app/logger.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QSet>
#include <QTcpServer>
#include <QTemporaryDir>
#include <QThread>
#include <QWebSocket>

#ifdef Q_OS_WIN
#include <QSettings>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace Cloudflare {
namespace {

QMutex                  g_mutex;
QHash<QString, bool>    g_blockedHosts;
QHash<QString, QString> g_hostUserAgents;

// Cancelled once when the app starts closing, so any solve in flight lets go.
const CancelToken g_shutdown;

const QString g_flareSolverrUrl    = qEnvironmentVariable("AONAMI_FLARESOLVERR");
const bool    g_browserSolveAllowed = qEnvironmentVariable("AONAMI_NO_BROWSER_SOLVE").isEmpty();

QString appFile(const char *name) {
    return QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(QLatin1String(name));
}

QString stripDot(QString domain) {
    if (domain.startsWith('.')) domain.remove(0, 1);
    return domain;
}

bool domainCovers(const QString &domain, const QString &host) {
    return host == domain || host.endsWith('.' + domain);
}

// Interstitial-only, so trusted at any status - challenges can arrive as 200.
constexpr const char *kInterstitialMarkers[] = {
    "window._cf_chl_opt",
    "cf-browser-verification",
    "just a moment...",
    // "/h/" is load-bearing - jsd/main.js is injected into ordinary pages too.
    "/cdn-cgi/challenge-platform/h/",
};

// Real pages carry these too - only trust them on an already-refused status.
constexpr const char *kChallengeMarkers[] = {
    "/cdn-cgi/challenge-platform",
    "challenges.cloudflare.com/turnstile",
    "cf_chl_opt",
    "enable javascript and cookies to continue",
    "checking your browser before accessing",
};

constexpr const char *kBlockMarkers[] = {
    "attention required! | cloudflare",
    "sorry, you have been blocked",
    "error 1015",           // rate limited
    "ray id:",
};

bool bodyHasAny(const QString &body, const char *const *markers, size_t count) {
    const QString head = body.left(kBodyScanBytes).toLower();
    for (size_t i = 0; i < count; ++i)
        if (head.contains(QLatin1String(markers[i]))) return true;
    return false;
}

QString headerOf(const QMap<QString, QString> &headers, const char *name) {
    for (auto it = headers.constBegin(); it != headers.constEnd(); ++it)
        if (it.key().compare(QLatin1String(name), Qt::CaseInsensitive) == 0) return it.value();
    return {};
}

bool servedByCloudflare(const QMap<QString, QString> &headers) {
    return headerOf(headers, "server").contains("cloudflare", Qt::CaseInsensitive) ||
           !headerOf(headers, "cf-ray").isEmpty();
}

bool isChallenged(int code, const QMap<QString, QString> &headers, const QString &body) {
    if (headerOf(headers, "cf-mitigated").compare("challenge", Qt::CaseInsensitive) == 0) return true;
    if (!servedByCloudflare(headers)) return false;
    if (bodyHasAny(body, kInterstitialMarkers, std::size(kInterstitialMarkers))) return true;
    if (code != 403 && code != 503 && code != 429) return false;
    return bodyHasAny(body, kChallengeMarkers, std::size(kChallengeMarkers));
}

void saveHostUserAgents() {
    QHash<QString, QString> copy;
    {
        QMutexLocker lock(&g_mutex);
        copy = g_hostUserAgents;
    }
    // QSaveFile: two saves can overlap, and a truncating write would interleave them.
    QSaveFile file(appFile("clearance-ua.txt"));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    for (auto it = copy.constBegin(); it != copy.constEnd(); ++it)
        file.write((it.key() + '\t' + it.value() + '\n').toUtf8());
    file.commit();
}

void loadHostUserAgents() {
    QFile file(appFile("clearance-ua.txt"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QMutexLocker lock(&g_mutex);
    while (!file.atEnd()) {
        const QByteArray line = file.readLine().trimmed();
        const int tab = line.indexOf('\t');
        if (tab > 0) g_hostUserAgents[QString::fromUtf8(line.left(tab))] = QString::fromUtf8(line.mid(tab + 1));
    }
}

void setHostUserAgent(const QString &host, const QString &userAgent) {
    if (host.isEmpty() || userAgent.isEmpty()) return;
    {
        QMutexLocker lock(&g_mutex);
        if (g_hostUserAgents.value(host) == userAgent) return;
        g_hostUserAgents[host] = userAgent;
    }
    saveHostUserAgents();
}

QNetworkCookie cookieFromJson(const QJsonObject &entry) {
    QNetworkCookie cookie(entry.value("name").toString().toUtf8(),
                          entry.value("value").toString().toUtf8());
    cookie.setDomain(entry.value("domain").toString());
    cookie.setPath(entry.value("path").toString("/"));
    cookie.setSecure(entry.value("secure").toBool());
    cookie.setHttpOnly(entry.value("httpOnly").toBool());
    if (const double expires = entry.value("expires").toDouble(-1); expires > 0)
        cookie.setExpirationDate(QDateTime::fromSecsSinceEpoch(qint64(expires)));
    return cookie;
}

// Site's own cookies only - a fresh profile signs into MSA and Bing, and none of
// that is ours to keep. extraHosts is wherever CF redirected us to.
QString storeSolution(const QUrl &url, const QList<QNetworkCookie> &cookies,
                      const QString &userAgent, const QSet<QString> &extraHosts) {
    QSet<QString> wanted = extraHosts;
    wanted.insert(url.host());

    QList<QNetworkCookie> ours;
    for (const QNetworkCookie &cookie : cookies) {
        const QString domain = stripDot(cookie.domain());
        if (domain.isEmpty()) continue;
        for (const QString &host : wanted) {
            if (!domainCovers(domain, host)) continue;
            ours.append(cookie);
            break;
        }
    }
    if (ours.isEmpty()) return {};

    CookieStore::instance().insert(ours);

    setHostUserAgent(url.host(), userAgent);
    for (const QNetworkCookie &cookie : std::as_const(ours))
        if (cookie.name() == "cf_clearance") setHostUserAgent(stripDot(cookie.domain()), userAgent);

    gLog() << "Cloudflare" << "cleared" << url.host() << "-" << QString::number(ours.size()) << "cookies";
    return userAgent;
}

QString solveViaFlareSolverr(const QUrl &url, int timeoutMs, QList<QNetworkCookie> &cookies) {
    const QJsonObject payload{
        {"cmd", "request.get"}, {"url", url.toString()}, {"maxTimeout", timeoutMs}};
    yLog() << "Cloudflare" << "asking FlareSolverr to solve" << url.host();

    Client client({}, false);
    client.setBypassEnabled(false);   // local solver; escalating here would loop
    const auto response = client.post(g_flareSolverrUrl, QJsonDocument(payload).toJson(QJsonDocument::Compact),
                                      {{"Content-Type", "application/json"}});

    const QJsonObject root = response.toJsonObject();
    if (root.value("status").toString() != "ok") {
        const QString message = root.value("message").toString();
        oLog() << "Cloudflare" << "FlareSolverr failed:" << (message.isEmpty() ? response.body.left(200) : message);
        return {};
    }
    const QJsonObject solution = root.value("solution").toObject();
    for (const QJsonValue &value : solution.value("cookies").toArray())
        cookies.append(cookieFromJson(value.toObject()));
    return solution.value("userAgent").toString();
}

// A managed challenge runs an obfuscated VM in the page - only a browser clears it.

QStringList candidateBrowsers() {
#ifdef Q_OS_WIN
    QStringList paths;
    for (const char *exe : {"msedge.exe", "chrome.exe", "brave.exe"})
        for (const char *root : {"HKEY_LOCAL_MACHINE", "HKEY_CURRENT_USER"}) {
            const QSettings key(QString("%1\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\%2")
                                    .arg(QLatin1String(root), QLatin1String(exe)), QSettings::NativeFormat);
            if (const QString path = key.value(".").toString(); !path.isEmpty()) paths << path;
        }
    return paths << "C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe"
                 << "C:/Program Files/Microsoft/Edge/Application/msedge.exe"
                 << "C:/Program Files/Google/Chrome/Application/chrome.exe";
#else
    return {"/Applications/Microsoft Edge.app/Contents/MacOS/Microsoft Edge",
            "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome",
            "/Applications/Brave Browser.app/Contents/MacOS/Brave Browser"};
#endif
}

QString browserPath() {
    for (const QString &path : candidateBrowsers())
        if (QFileInfo::exists(path)) return path;
    return {};
}

// shutdown() kills these outright - whatever a solve is blocked on, losing its
// browser unblocks it, the same way closing the window by hand did.
QMutex          g_browsersMutex;
QSet<qint64>    g_browsers;

void killPid(qint64 pid) {
#ifdef Q_OS_WIN
    if (HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, DWORD(pid))) {
        TerminateProcess(h, 1);
        CloseHandle(h);
    }
#else
    Q_UNUSED(pid);
#endif
}

void forgetBrowser(qint64 pid) {
    QMutexLocker lock(&g_browsersMutex);
    g_browsers.remove(pid);
}

void killAllBrowsers() {
    QSet<qint64> live;
    {
        QMutexLocker lock(&g_browsersMutex);
        live.swap(g_browsers);
    }
    for (const qint64 pid : live) killPid(pid);
}

// Windows closes our handles however we die, and closing the job kills everything
// in it - so even a crash mid-solve can't leave a browser behind.
void tieToOurLifetime(qint64 pid) {
#ifdef Q_OS_WIN
    static HANDLE job = [] {
        HANDLE h = CreateJobObjectW(nullptr, nullptr);
        if (!h) return HANDLE(nullptr);
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
        limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        SetInformationJobObject(h, JobObjectExtendedLimitInformation, &limits, sizeof(limits));
        return h;
    }();
    if (!job || pid <= 0) return;
    if (HANDLE child = OpenProcess(PROCESS_SET_QUOTA | PROCESS_TERMINATE, FALSE, DWORD(pid))) {
        AssignProcessToJobObject(job, child);
        CloseHandle(child);
    }
#else
    Q_UNUSED(pid);
#endif
}

QJsonDocument cdpGet(int port, const char *path) {
    Client client({}, false);
    client.setBypassEnabled(false);
    return QJsonDocument::fromJson(
        client.get(QString("http://127.0.0.1:%1/%2").arg(port).arg(QLatin1String(path))).body.toUtf8());
}

// Where the clearance really landed, after CF's redirects.
QSet<QString> openPageHosts(int port) {
    QSet<QString> hosts;
    for (const QJsonValue &value : cdpGet(port, "json/list").array()) {
        const QJsonObject target = value.toObject();
        if (target.value("type").toString() != "page") continue;
        if (const QString host = QUrl(target.value("url").toString()).host(); !host.isEmpty())
            hosts.insert(host);
    }
    return hosts;
}

QString solveViaBrowser(const QUrl &url, const CancelToken &cancel, int timeoutMs) {
    // The caller giving up and the app closing, checked together everywhere below.
    const auto abandoned = [&cancel] { return cancel.isCancelled() || g_shutdown.isCancelled(); };

    const QString browser = browserPath();
    QTemporaryDir profile;
    if (browser.isEmpty() || !profile.isValid() || abandoned()) return {};

    int port = 0;
    {
        QTcpServer probe;   // 0 = let the OS pick; races the browser's bind, but beats guessing
        if (!probe.listen(QHostAddress::LocalHost, 0)) return {};
        port = probe.serverPort();
    }

    QProcess process;
    process.setProgram(browser);
    process.setArguments({
        // Never --headless, CF detects it - a real window, parked off-screen.
        "--window-position=-32000,-32000",
        "--remote-debugging-port=" + QString::number(port),
        "--user-data-dir=" + profile.path(),   // Chromium won't expose the port on the real profile
        "--remote-allow-origins=*",            // Chromium 111+ rejects the WS handshake otherwise
        "--no-first-run", "--no-default-browser-check", "--disable-background-networking",
        // A fresh profile otherwise signs into the Windows account, dragging its cookies in.
        "--disable-sync", "--no-service-autorun", "--disable-background-mode",
        "--disable-features=msImplicitSignin,msEdgeImplicitSignin,EdgeImplicitSignIn,"
        "msEdgeSyncPromo,msSignInPromo,AccountConsistency",
        "--window-size=1280,800", url.toString(),
    });
    process.setStandardOutputFile(QProcess::nullDevice());
    process.setStandardErrorFile(QProcess::nullDevice());
    process.start();
    if (!process.waitForStarted(10000)) return {};
    const qint64 browserPid = process.processId();
    tieToOurLifetime(browserPid);
    { QMutexLocker lock(&g_browsersMutex); g_browsers.insert(browserPid); }
    // shutdown() may already have fired while the browser was starting.
    if (abandoned()) { killPid(browserPid); forgetBrowser(browserPid); return {}; }

    yLog() << "Cloudflare" << "solving" << url.host();

    QElapsedTimer clock;
    clock.start();

    // Browser-level endpoint - hands us the UA to bind, and takes Browser.close.
    QString socketUrl, userAgent;
    while (socketUrl.isEmpty() && clock.elapsed() < timeoutMs && !abandoned()
           && process.state() != QProcess::NotRunning) {
        const QJsonObject version = cdpGet(port, "json/version").object();
        socketUrl = version.value("webSocketDebuggerUrl").toString();
        userAgent = version.value("User-Agent").toString();
        if (socketUrl.isEmpty()) QThread::msleep(200);
    }

    QWebSocket socket;
    QList<QNetworkCookie> harvested;
    int nextId = 1;

    if (!socketUrl.isEmpty()) {
        QObject::connect(&socket, &QWebSocket::textMessageReceived, &socket, [&](const QString &message) {
            const QJsonArray cookies = QJsonDocument::fromJson(message.toUtf8())
                                           .object().value("result").toObject().value("cookies").toArray();
            QList<QNetworkCookie> found;
            bool cleared = false;
            for (const QJsonValue &value : cookies) {
                const QJsonObject entry = value.toObject();
                found.append(cookieFromJson(entry));
                cleared |= entry.value("name").toString() == "cf_clearance";
            }
            if (cleared) harvested = found;
        });

        // Spun rather than exec()'d: the browser may never clear, and every pass has to
        // re-check the deadline, the caller giving up and the app closing.
        socket.open(QUrl(socketUrl));
        qint64 lastPoll = 0;
        while (harvested.isEmpty() && clock.elapsed() < timeoutMs && !abandoned()
               && process.state() != QProcess::NotRunning) {
            if (socket.state() == QAbstractSocket::ConnectedState
                && clock.elapsed() - lastPoll >= 500) {
                lastPoll = clock.elapsed();
                socket.sendTextMessage(QString(R"({"id":%1,"method":"Storage.getCookies"})").arg(nextId++));
            }
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
            QThread::msleep(10);
        }
    }

    const QSet<QString> hosts = harvested.isEmpty() ? QSet<QString>{} : openPageHosts(port);

    // Killing the launcher orphans its children and leaves the window up, so ask it to
    // close - unless we're being abandoned, where the job object mops up instead.
    if (!abandoned() && socket.state() == QAbstractSocket::ConnectedState) {
        socket.sendTextMessage(QString(R"({"id":%1,"method":"Browser.close"})").arg(nextId++));
        process.waitForFinished(3000);
    }
    socket.close();
    if (process.state() != QProcess::NotRunning) {
        process.kill();
        process.waitForFinished(abandoned() ? 500 : 3000);
    }
    killPid(browserPid);          // the launcher can exit before the browser it spawned
    forgetBrowser(browserPid);

    if (harvested.isEmpty()) {
        oLog() << "Cloudflare" << "challenge not cleared for" << url.host()
               << "- the site may be hard-blocking this IP";
        return {};
    }

    const QString bound = storeSolution(url, harvested, userAgent, hosts);
    if (bound.isEmpty()) oLog() << "Cloudflare" << "solved but no cookies scoped to" << url.host();
    else                 gLog() << "Cloudflare" << "solved in"
                                << QString::number(clock.elapsed() / 1000.0, 'f', 1) + "s";
    return bound;
}

} // namespace

bool isBlocked(int code, const QMap<QString, QString> &headers, const QString &body) {
    if (isChallenged(code, headers, body)) return true;
    if (code != 403 && code != 503 && code != 429) return false;
    if (!servedByCloudflare(headers)) return false;
    // A bare 403 from the edge is a fingerprint rejection; the origin's has a page.
    return body.isEmpty() || bodyHasAny(body, kBlockMarkers, std::size(kBlockMarkers));
}

void markBlocked(const QString &host) {
    if (host.isEmpty()) return;
    QMutexLocker lock(&g_mutex);
    if (g_blockedHosts.value(host, false)) return;
    g_blockedHosts[host] = true;
    lock.unlock();
    yLog() << "Cloudflare" << host << "refused the direct client";
}


QString hostUserAgent(const QString &host) {
    QMutexLocker lock(&g_mutex);
    if (const QString exact = g_hostUserAgents.value(host); !exact.isEmpty()) return exact;

    // animepahe.pw's clearance covers i.animepahe.pw
    for (QString parent = host;;) {
        const int dot = parent.indexOf('.');
        if (dot < 0) return {};
        parent = parent.mid(dot + 1);
        if (!parent.contains('.')) return {};   // stop before a bare public suffix
        if (const QString match = g_hostUserAgents.value(parent); !match.isEmpty()) return match;
    }
}

static QString takeHeader(QMap<QString, QString> &headers, const char *name) {
    for (auto it = headers.begin(); it != headers.end(); ++it) {
        if (it.key().compare(QLatin1String(name), Qt::CaseInsensitive) != 0) continue;
        const QString value = it.value();
        headers.erase(it);
        return value;
    }
    return {};
}

void applyClearanceHeaders(const QUrl &url, QMap<QString, QString> &headers) {
    const QString existing = takeHeader(headers, "cookie");
    if (const QByteArray merged = CookieStore::instance().cookieHeader(url, existing); !merged.isEmpty())
        headers["Cookie"] = QString::fromUtf8(merged);

    if (const QString userAgent = hostUserAgent(url.host()); !userAgent.isEmpty()) {
        takeHeader(headers, "user-agent");
        headers["User-Agent"] = userAgent;
    }
}

bool canSolveChallenge() {
    if (g_shutdown.isCancelled()) return false;
    return !g_flareSolverrUrl.isEmpty() || (g_browserSolveAllowed && !browserPath().isEmpty());
}

void shutdown() {
    g_shutdown.cancel();
    killAllBrowsers();   // don't wait for the solve to notice - take its browser away
    CookieStore::instance().flush();
}

QString solveChallenge(const QUrl &url, const CancelToken &cancel, int timeoutMs) {
    if (!canSolveChallenge() || cancel.isCancelled()) return {};

    // Neither backend takes two at once. A host that just failed is left alone for a
    // good while, or an unsolvable site is retried forever.
    static QMutex solveMutex;
    static QHash<QString, qint64> retryAfterMs;
    QMutexLocker lock(&solveMutex);
    if (g_shutdown.isCancelled() || cancel.isCancelled()) return {};

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now < retryAfterMs.value(url.host(), 0)) return hostUserAgent(url.host());

    QString userAgent;
    if (g_flareSolverrUrl.isEmpty()) {
        userAgent = solveViaBrowser(url, cancel, timeoutMs);
    } else {
        QList<QNetworkCookie> cookies;
        userAgent = solveViaFlareSolverr(url, timeoutMs, cookies);
        if (!userAgent.isEmpty()) userAgent = storeSolution(url, cookies, userAgent, {});
    }

    retryAfterMs[url.host()] = QDateTime::currentMSecsSinceEpoch() + (userAgent.isEmpty() ? 600000 : 30000);
    return userAgent;
}

CookieStore &CookieStore::instance() {
    static CookieStore store;
    static const bool loaded = (store.load(), loadHostUserAgents(), true);
    Q_UNUSED(loaded);
    return store;
}

QList<QNetworkCookie> CookieStore::cookiesForUrl(const QUrl &url) const {
    QMutexLocker lock(&m_mutex);
    return m_jar.cookiesForUrl(url);
}

void CookieStore::setCookiesFromUrl(const QList<QNetworkCookie> &cookies, const QUrl &url) {
    if (cookies.isEmpty()) return;

    // save() writes persistent cookies only, so session churn - nearly all of this
    // traffic - needs no disk write at all.
    auto persistent = [](QList<QNetworkCookie> all) {
        all.removeIf([](const QNetworkCookie &c) { return c.isSessionCookie(); });
        return all;
    };
    bool changed = false;
    {
        QMutexLocker lock(&m_mutex);
        const auto before = persistent(m_jar.allCookies());
        m_jar.setCookiesFromUrl(cookies, url);
        changed = persistent(m_jar.allCookies()) != before;
    }
    if (!changed) return;

    // Some sites re-issue a persistent cookie on every response, so rate-limit the write.
    // Deferring rather than dropping it matters - flush() at exit picks up the last one.
    {
        QMutexLocker lock(&m_mutex);
        if (QDateTime::currentMSecsSinceEpoch() - m_lastSaveMs < 5000) {
            m_pendingSave = true;
            return;
        }
    }
    save();
}

void CookieStore::flush() const {
    QMutexLocker lock(&m_mutex);
    if (!m_pendingSave) return;
    lock.unlock();
    save();
}

void CookieStore::insert(const QList<QNetworkCookie> &cookies) {
    if (cookies.isEmpty()) return;
    {
        QMutexLocker lock(&m_mutex);
        QList<QNetworkCookie> all = m_jar.allCookies();
        for (const QNetworkCookie &cookie : cookies) {
            all.removeIf([&cookie](const QNetworkCookie &e) {
                return e.name() == cookie.name() && e.domain() == cookie.domain() && e.path() == cookie.path();
            });
            all.append(cookie);
        }
        m_jar.setAllCookies(all);
    }
    save();
}

QByteArray CookieStore::cookieHeader(const QUrl &url, const QString &existing) const {
    QList<std::pair<QByteArray, QByteArray>> pairs;   // caller's entries keep order
    QHash<QByteArray, int> index;

    for (const QByteArray &chunk : existing.trimmed().toUtf8().split(';')) {
        const QByteArray entry = chunk.trimmed();
        if (entry.isEmpty()) continue;
        const int eq = entry.indexOf('=');
        const QByteArray name = (eq < 0 ? entry : entry.left(eq)).trimmed();
        if (name.isEmpty()) continue;
        index.insert(name, int(pairs.size()));
        pairs.append({name, eq < 0 ? QByteArray() : entry.mid(eq + 1).trimmed()});
    }

    for (const QNetworkCookie &cookie : cookiesForUrl(url)) {
        const auto it = index.constFind(cookie.name());
        if (it == index.constEnd()) {
            index.insert(cookie.name(), int(pairs.size()));
            pairs.append({cookie.name(), cookie.value()});
        } else if (pairs[*it].second.isEmpty()) {
            pairs[*it].second = cookie.value();   // fill the caller's empty placeholder
        }
    }

    QByteArray header;
    for (const auto &[name, value] : std::as_const(pairs)) {
        if (!header.isEmpty()) header += "; ";
        header += name + '=' + value;
    }
    return header;
}

void CookieStore::load() {
    QFile file(appFile("cookies.txt"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    QList<QNetworkCookie> cookies;
    const QDateTime now = QDateTime::currentDateTimeUtc();
    while (!file.atEnd()) {
        // Expired ones get sent, get refused, and hide that we never had access.
        for (const QNetworkCookie &cookie : QNetworkCookie::parseCookies(file.readLine().trimmed()))
            if (cookie.isSessionCookie() || cookie.expirationDate() > now) cookies.append(cookie);
    }

    QMutexLocker lock(&m_mutex);
    m_jar.setAllCookies(cookies);
    if (cookies.isEmpty()) return;
    lock.unlock();
    gLog() << "Cloudflare" << "restored" << QString::number(cookies.size()) << "cookies";
}

void CookieStore::save() const {
    QList<QNetworkCookie> cookies;
    {
        QMutexLocker lock(&m_mutex);
        cookies = m_jar.allCookies();
        m_pendingSave = false;
        m_lastSaveMs  = QDateTime::currentMSecsSinceEpoch();
    }
    QSaveFile file(appFile("cookies.txt"));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    for (const QNetworkCookie &cookie : std::as_const(cookies))
        if (!cookie.isSessionCookie())   // nothing to carry across a restart
            file.write(cookie.toRawForm(QNetworkCookie::Full) + '\n');
    file.commit();
}

QList<QNetworkCookie> ProxyCookieJar::cookiesForUrl(const QUrl &url) const {
    return CookieStore::instance().cookiesForUrl(url);
}

bool ProxyCookieJar::setCookiesFromUrl(const QList<QNetworkCookie> &cookies, const QUrl &url) {
    CookieStore::instance().setCookiesFromUrl(cookies, url);
    return true;
}

} // namespace Cloudflare
