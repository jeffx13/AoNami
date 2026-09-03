#include "net/cloudflare.h"
#include "net/client.h"
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

#include <QSettings>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace Cloudflare {
namespace {

QMutex                  g_mutex;
QSet<QString>           g_loggedBlocks;   // one "refused" line per host, not per request
QHash<QString, QString> g_hostUserAgents;

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

// Site's own cookies only: a fresh profile signs into MSA and Bing, and none of that is ours.
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

    logOk() << "Cloudflare" << "cleared" << url.host() << "-" << QString::number(ours.size()) << "cookies";
    return userAgent;
}

QString solveViaFlareSolverr(const QUrl &url, int timeoutMs, QList<QNetworkCookie> &cookies) {
    const QJsonObject payload{
        {"cmd", "request.get"}, {"url", url.toString()}, {"maxTimeout", timeoutMs}};
    logStep() << "Cloudflare" << "asking FlareSolverr to solve" << url.host();

    Client client({}, false);
    client.setBypassEnabled(false);   // local solver; escalating here would loop
    const auto response = client.post(g_flareSolverrUrl, QJsonDocument(payload).toJson(QJsonDocument::Compact),
                                      {{"Content-Type", "application/json"}});

    const QJsonObject root = response.toJsonObject();
    if (root.value("status").toString() != "ok") {
        const QString message = root.value("message").toString();
        logWarn() << "Cloudflare" << "FlareSolverr failed:" << (message.isEmpty() ? response.body.left(200) : message);
        return {};
    }
    const QJsonObject solution = root.value("solution").toObject();
    for (const QJsonValue &value : solution.value("cookies").toArray())
        cookies.append(cookieFromJson(value.toObject()));
    return solution.value("userAgent").toString();
}

// A managed challenge runs an obfuscated VM in the page - only a browser clears it.

QStringList candidateBrowsers() {
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
}

QString browserPath() {
    for (const QString &path : candidateBrowsers())
        if (QFileInfo::exists(path)) return path;
    return {};
}

// Losing its browser unblocks a solve whatever it is stuck on.
QMutex          g_browsersMutex;
QSet<qint64>    g_browsers;
HANDLE          g_browserJob = nullptr;   // guarded by g_browsersMutex

void killPid(qint64 pid) {
    if (pid <= 0) return;
    if (HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, DWORD(pid))) {
        TerminateProcess(h, 1);
        CloseHandle(h);
    }
}

void forgetBrowser(qint64 pid) {
    QMutexLocker lock(&g_browsersMutex);
    g_browsers.remove(pid);
}

// Killing the pid leaves Chromium's children; closing the job takes the tree, and must happen here.
void killAllBrowsers() {
    QSet<qint64> live;
    {
        QMutexLocker lock(&g_browsersMutex);
        live.swap(g_browsers);
    }
    for (const qint64 pid : live) killPid(pid);
    HANDLE job = nullptr;
    {
        QMutexLocker lock(&g_browsersMutex);
        std::swap(job, g_browserJob);   // a later solve makes a fresh one
    }
    if (job) CloseHandle(job);
    if (!live.isEmpty())
        logStep() << "Cloudflare" << "took down" << QString::number(live.size()) << "browser(s)";
}

// Windows closes our handles however we die, so even a crash can't leave a browser.
void tieToOurLifetime(qint64 pid) {
    if (pid <= 0) return;
    QMutexLocker lock(&g_browsersMutex);
    if (!g_browserJob) {
        g_browserJob = CreateJobObjectW(nullptr, nullptr);
        if (!g_browserJob) return;
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
        limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        SetInformationJobObject(g_browserJob, JobObjectExtendedLimitInformation, &limits, sizeof(limits));
    }
    if (HANDLE child = OpenProcess(PROCESS_SET_QUOTA | PROCESS_TERMINATE, FALSE, DWORD(pid))) {
        if (!AssignProcessToJobObject(g_browserJob, child))
            logWarn() << "Cloudflare" << "could not tie the browser to this process";
        CloseHandle(child);
    }
}

QJsonDocument cdpGet(int port, const char *path) {
    Client client({}, false);
    client.setBypassEnabled(false);
    return QJsonDocument::fromJson(
        client.get(QString("http://127.0.0.1:%1/%2").arg(port).arg(QLatin1String(path))).body.toUtf8());
}

// A challenge announces itself in the title; anything else will never yield a clearance.
constexpr const char *kChallengeTitles[] = {
    "just a moment",
    "checking your browser",
    "verifying you are human",
    "please wait",
};

bool challengeInFlight(int port) {
    for (const QJsonValue &value : cdpGet(port, "json/list").array()) {
        const QJsonObject target = value.toObject();
        if (target.value("type").toString() != "page") continue;
        const QString title = target.value("title").toString().toLower();
        for (const char *marker : kChallengeTitles)
            if (title.contains(QLatin1String(marker))) return true;
    }
    return false;
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
    // It may have launched anyway, and would then leak: no pid recorded, never tied to the job.
    if (!process.waitForStarted(10000)) {
        killPid(process.processId());
        process.kill();
        process.waitForFinished(2000);
        return {};
    }
    const qint64 browserPid = process.processId();
    tieToOurLifetime(browserPid);
    { QMutexLocker lock(&g_browsersMutex); g_browsers.insert(browserPid); }
    if (abandoned()) { killPid(browserPid); forgetBrowser(browserPid); return {}; }

    logStep() << "Cloudflare" << "solving" << url.host() << "(browser pid" << QString::number(browserPid) + ")";

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
    bool deadEnd = false;

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

        // Spun rather than exec()'d so every pass rechecks the deadline and both cancels.
        socket.open(QUrl(socketUrl));
        qint64 lastPoll = 0, lastCheck = 0;
        int quiet = 0;   // consecutive checks with no challenge on screen
        while (harvested.isEmpty() && clock.elapsed() < timeoutMs && !abandoned()
               && process.state() != QProcess::NotRunning) {
            if (socket.state() == QAbstractSocket::ConnectedState
                && clock.elapsed() - lastPoll >= 500) {
                lastPoll = clock.elapsed();
                socket.sendTextMessage(QString(R"({"id":%1,"method":"Storage.getCookies"})").arg(nextId++));
            }
            // Cookies keep polling between these, so a solve landing on the last check still gets out.
            if (clock.elapsed() > 6000 && clock.elapsed() - lastCheck >= 2000) {
                lastCheck = clock.elapsed();
                quiet = challengeInFlight(port) ? 0 : quiet + 1;
                if (quiet >= 3) { deadEnd = true; break; }
            }
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
            QThread::msleep(10);
        }
    }

    const QSet<QString> hosts = harvested.isEmpty() ? QSet<QString>{} : openPageHosts(port);

    // Killing the launcher orphans its children, so ask nicely unless the job object is mopping up.
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
    logInfo() << "Cloudflare" << "browser pid" << QString::number(browserPid) << "closed";

    if (harvested.isEmpty()) {
        if (deadEnd) logWarn() << "Cloudflare" << url.host() << "showed no challenge - refused outright, or no clearance to give";
        else         logWarn() << "Cloudflare" << "challenge not cleared for" << url.host()
                            << "- the site may be hard-blocking this IP";
        return {};
    }

    const QString bound = storeSolution(url, harvested, userAgent, hosts);
    if (bound.isEmpty()) logWarn() << "Cloudflare" << "solved but no cookies scoped to" << url.host();
    else                 logOk() << "Cloudflare" << "solved in"
                                << QString::number(clock.elapsed() / 1000.0, 'f', 1) + "s";
    return bound;
}

}

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
    if (g_loggedBlocks.contains(host)) return;
    g_loggedBlocks.insert(host);
    lock.unlock();
    logStep() << "Cloudflare" << host << "refused the direct client";
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

void shutdown() {
    g_shutdown.cancel();
    killAllBrowsers();
    CookieStore::instance().flush();
}

QString solveChallenge(const QUrl &url, const CancelToken &cancel, int timeoutMs) {
    const bool haveSolver = !g_flareSolverrUrl.isEmpty()
                            || (g_browserSolveAllowed && !browserPath().isEmpty());
    if (!haveSolver || g_shutdown.isCancelled() || cancel.isCancelled()) return {};

    // Neither backend takes two at once, and a host that just failed is left alone for a good while.
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

    // save() writes persistent cookies only, so session churn needs no disk write at all.
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

    // Some sites re-issue a persistent cookie every response; defer rather than drop, flush() gets the last.
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
    logOk() << "Cloudflare" << "restored" << QString::number(cookies.size()) << "cookies";
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

}
