#include "mpvplayer.h"
#include "settings.h"
#include <QDir>
#include <QCoreApplication>
#include <QMetaType>
#include <QOpenGLContext>
#include <QStandardPaths>
#include <clocale>
#include <stdexcept>
#include <limits>
#include <QStringList>
#include "core/utils/displaysleep.h"
#include <QQuickOpenGLUtils>
#include <QtOpenGL/QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QElapsedTimer>
#include <QGuiApplication>
#include <QScreen>
#include "ui/uibridge.h"
#include "app/logger.h"

// mpv renders into the item's own FBO on the scene-graph thread, sharing Qt's GL context. Giving it
// a private thread and a second context instead makes the driver sync on every shader pass.
class MpvRenderer : public QQuickFramebufferObject::Renderer {
    MpvPlayer *m_obj;
    bool m_visible = true;

public:
    MpvRenderer(MpvPlayer *obj) : m_obj(obj) {}
    ~MpvRenderer() override { m_obj->freeMpvRenderContext(); }

    void synchronize(QQuickFramebufferObject *item) override { m_visible = item->isVisible(); }

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override {
        QOpenGLFramebufferObjectFormat fmt;
        fmt.setAttachment(QOpenGLFramebufferObject::NoAttachment);
        fmt.setSamples(0);
        fmt.setMipmap(false);
        return new QOpenGLFramebufferObject(m_obj->clampRenderSize(size), fmt);
    }

    void render() override {
        QOpenGLFramebufferObject *target = framebufferObject();
        if (!target || !m_obj->ensureMpvRenderContext()) return;

        QQuickOpenGLUtils::resetOpenGLState();
        if (!m_visible) {
            auto *gl = QOpenGLContext::currentContext()->functions();
            gl->glClearColor(0.f, 0.f, 0.f, 1.f);
            gl->glClear(GL_COLOR_BUFFER_BIT);
            QQuickOpenGLUtils::resetOpenGLState();
            return;
        }

        mpv_opengl_fbo mpfbo{static_cast<int>(target->handle()), target->width(), target->height(), 0};
        int flip_y = 0;
        // The scene graph presents, not mpv; left at its default it sleeps until the frame is due and
        // hands it over already late, then drops the next one.
        int blockForTargetTime = 0;
        mpv_render_param params[] = {
            {MPV_RENDER_PARAM_OPENGL_FBO, &mpfbo},
            {MPV_RENDER_PARAM_FLIP_Y, &flip_y},
            {MPV_RENDER_PARAM_BLOCK_FOR_TARGET_TIME, &blockForTargetTime},
            {MPV_RENDER_PARAM_INVALID, nullptr}
        };
        m_obj->handle().render(params);
        QQuickOpenGLUtils::resetOpenGLState();
    }
};

MpvPlayer::MpvPlayer(QQuickItem *parent) : QQuickFramebufferObject(parent) {
    s_instance.store(this, std::memory_order_release);
    // Cap the render size to the display (GUI thread - QScreen isn't safe off it).
    QSize cap;
    if (QScreen *s = QGuiApplication::primaryScreen())
        cap = (QSizeF(s->size()) * s->devicePixelRatio()).toSize();
    if (cap.isEmpty()) cap = QSize(1920, 1080);
    m_maxRenderSize.store(cap.boundedTo(QSize(3840, 3840)), std::memory_order_relaxed);
    m_time.store(0, std::memory_order_relaxed);
    m_duration.store(0, std::memory_order_relaxed);
    int vol = Settings::instance().get(Config::Volume);
    m_volume = (vol >= 0 && vol <= 200) ? vol : 100;
    double speed = Settings::instance().get(Config::Speed);
    m_speed = (speed > 0.0 && speed <= 10.0) ? static_cast<float>(speed) : 1.0f;

    connect(this, &QQuickItem::windowChanged, this, [this]() { recomputeMaxRenderSize(); });
    // Bundled mpv folder next to the exe; fall back to %APPDATA%/mpv for user overrides.
    QDir mpvDir(QCoreApplication::applicationDirPath() + "/mpv");
    if (!mpvDir.exists())
        mpvDir = QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/../mpv");
    if (mpvDir.exists()) {
        m_mpv.set_option("config-dir", mpvDir.absolutePath().toLocal8Bit().constData());
        m_mpv.set_option("config", "yes");
    }

    m_mpv.set_option("ytdl", Settings::instance().mpvYtdlEnabled());
    m_mpv.set_option("pause", false);
    m_mpv.set_option("softvol", true);
    m_mpv.set_option("vo", "libmpv");
    m_mpv.set_option("keep-open", true);
    m_mpv.set_option("screenshot-directory",
                     QStandardPaths::writableLocation(QStandardPaths::PicturesLocation).toUtf8().constData());

    m_mpv.set_option("reset-on-next-file", "video-aspect-override,af,audio-delay,pause");

    m_mpv.set_option("hwdec", "auto");

    m_mpv.set_option("cache", "yes");
    m_mpv.set_option("cache-secs", "100");
    m_mpv.set_option("cache-unlink-files", "whendone");
    const bool mpvLogOn = Settings::instance().mpvLogEnabled();
    m_mpv.set_option("msg-level", mpvLogOn ? "all=v" : "all=error");
    m_mpv.set_option("force-seekable", "yes");
    m_mpv.set_option("cookies", "no");
    // Some CDNs serve valid streams behind certs the OS distrusts (Client ignores them too).
    m_mpv.set_option("tls-verify", "no");
    // Let sub-scale also resize ASS/styled subs (sub-font-size alone is ignored for those).
    m_mpv.set_option("sub-ass-override", "scale");

    m_mpv.observe_property("duration");
    m_mpv.observe_property("playback-time");
    m_mpv.observe_property("paused-for-cache");
    m_mpv.observe_property("base-idle");
    m_mpv.observe_property("pause");
    m_mpv.observe_property("track-list");
    m_mpv.observe_property("glsl-shaders");
    m_mpv.observe_property("aid");
    m_mpv.observe_property("sid");
    m_mpv.observe_property("vid");
    m_mpv.request_log_messages(mpvLogOn ? "info" : "error");

    QObject::connect(&Settings::instance(), &Settings::mpvYtdlEnabledChanged, this, [this]() {
        bool enabled = Settings::instance().mpvYtdlEnabled();
        m_mpv.set_property_async("ytdl", enabled);
        showText(QString("ytdl: %1").arg(enabled ? "on" : "off"));
    });

    QObject::connect(&Settings::instance(), &Settings::mpvLogEnabledChanged, this, [this]() {
        bool enabled = Settings::instance().mpvLogEnabled();
        m_mpv.set_property_async("msg-level", enabled ? "all=v" : "all=error");
        m_mpv.request_log_messages(enabled ? "info" : "error");
    });

    if (Settings::instance().get(Config::LimitCache)) {
        int64_t forwardBytes  = Settings::instance().get(Config::ForwardCache)  << 20;
        int64_t backwardBytes = Settings::instance().get(Config::BackwardCache) << 20;
        m_mpv.set_option("demuxer-max-bytes", forwardBytes);
        m_mpv.set_option("demuxer-max-back-bytes", backwardBytes);
    }

    if (m_mpv.initialize() < 0)
        throw std::runtime_error("could not initialize mpv context");

    m_mpv.set_property("volume", static_cast<double>(m_volume));
    m_mpv.set_property("speed",  static_cast<double>(m_speed));

    m_mpv.set_wakeup_callback(
        [](void *ctx) {
            MpvPlayer *obj = static_cast<MpvPlayer *>(ctx);
            QMetaObject::invokeMethod(obj, "onMpvEvent", Qt::QueuedConnection);
        },
        this);

}

MpvPlayer::~MpvPlayer() {
    s_instance.store(nullptr, std::memory_order_release);
    const char *stopCmd[] = {"stop", nullptr};
    m_mpv.command(stopCmd);   // kill decode + audio now; terminate_destroy alone lets it play on
}

void MpvPlayer::recomputeMaxRenderSize() {
    QScreen *s = window() ? window()->screen() : QGuiApplication::primaryScreen();
    QSize sz = s ? (QSizeF(s->size()) * s->devicePixelRatio()).toSize() : QSize();
    if (sz.isEmpty()) sz = QSize(1920, 1080);
    sz = sz.boundedTo(QSize(3840, 3840));
    if (sz == m_maxRenderSize.load(std::memory_order_relaxed)) return;
    m_maxRenderSize.store(sz, std::memory_order_relaxed);
    if (window()) connect(window(), &QWindow::screenChanged, this,
                          [this]() { recomputeMaxRenderSize(); update(); }, Qt::UniqueConnection);
    update();
}

bool MpvPlayer::ensureMpvRenderContext() {
    if (m_renderCtxInited) return true;

    mpv_opengl_init_params gl_init {
        [](void *, const char *name) -> void * {
            QOpenGLContext *c = QOpenGLContext::currentContext();
            return c ? reinterpret_cast<void *>(c->getProcAddress(QByteArray(name))) : nullptr;
        }
#if MPV_CLIENT_API_VERSION < MPV_MAKE_VERSION(2, 0)
        , nullptr, nullptr
#endif
    };
    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_API_TYPE, const_cast<char *>(MPV_RENDER_API_TYPE_OPENGL)},
        {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_init},
        {MPV_RENDER_PARAM_INVALID, nullptr}
    };
    if (m_mpv.renderer_initialize(params) < 0) {
        rLog() << "Mpv" << "renderer_initialize failed";
        return false;
    }
    m_mpv.set_render_callback([](void *ctx) {
        auto *self = static_cast<MpvPlayer *>(ctx);
        QMetaObject::invokeMethod(self, [self]() { self->update(); }, Qt::QueuedConnection);
    }, this);
    m_renderCtxInited = true;
    return true;
}

void MpvPlayer::freeMpvRenderContext() {
    if (!m_renderCtxInited) return;
    m_renderCtxInited = false;
    m_mpv.set_render_callback(nullptr, nullptr);
    m_mpv.free_renderer();
}

QSize MpvPlayer::clampRenderSize(QSize itemPx) const {
    const QSize cap = m_maxRenderSize.load(std::memory_order_relaxed);
    if (itemPx.isEmpty()) return cap;

    QSize sz = itemPx;
    if (sz.width() > cap.width() || sz.height() > cap.height())
        sz = sz.scaled(cap, Qt::KeepAspectRatio);

    // Anime4K's AutoDownscalePre passes only fire for OUTPUT.w/NATIVE.w inside (1.2, 2.0) or
    // (2.4, 4.0); outside those the final CNN pass runs at 16x NATIVE instead. Opening the sidebar
    // is enough to cross the 1.2 edge, so nudge back into the nearest band - upwards only, since a
    // texture that downscales stays sharp and one that upscales does not.
    const int nativeW = m_videoWidth.load(std::memory_order_relaxed);
    if (m_bandClamp.load(std::memory_order_relaxed) && nativeW > 0) {
        const double r = double(sz.width()) / double(nativeW);
        const double target = (r < 1.26) ? 1.26 : (r >= 1.99 && r < 2.46 ? 2.46 : 0.0);
        if (target > r && target <= r * 2.0) {
            sz = (QSizeF(sz) * (target / r)).toSize();
            if (sz.width() > 4096 || sz.height() > 4096)
                sz = sz.scaled(QSize(4096, 4096), Qt::KeepAspectRatio);
        }
    }

    if (sz.width() < 64 || sz.height() < 64) {
        const double s = qMax(64.0 / qMax(1, sz.width()), 64.0 / qMax(1, sz.height()));
        sz = (QSizeF(sz) * s).toSize();
    }
    return sz;
}

void MpvPlayer::open(PlayInfo &playItem) {
    if (playItem.videos.isEmpty()) return;

    setLoading(true);

    m_state = STOPPED;
    emit mpvStateChanged();

    m_seekTime = playItem.timestamp;
    std::stable_sort(playItem.videos.begin(), playItem.videos.end(),
                     [](const Video &a, const Video &b) {
                         if (a.resolution != b.resolution) return a.resolution > b.resolution;
                         return a.bitrate > b.bitrate;
                     });

    // DASH gives audio as its own stream; best bitrate wins.
    std::stable_sort(playItem.audios.begin(), playItem.audios.end(),
                     [](const Track &a, const Track &b) {
                         return a.bitrate > b.bitrate;
                     });

    setHeaders(playItem.headers);

    QByteArray videoUrlData = (playItem.videos[0].url.isLocalFile()
                                   ? playItem.videos[0].url.toLocalFile()
                                   : playItem.videos[0].url.toString()).toUtf8();
    const char *args[] = {"loadfile", videoUrlData.constData(), nullptr};
    m_mpv.command_async(args);

    m_audiosToBeAdded = playItem.audios;
    m_subtitlesToBeAdded = playItem.subtitles;
    m_videosToBeAdded = playItem.videos;

    m_currentVideoUrl = playItem.videos[0].url;
    UiBridge::instance().navigateTo(UiBridge::Page::Player);
}

void MpvPlayer::play() {
    if (m_state == VIDEO_PAUSED)
        m_mpv.set_property_async("pause", false);
}

void MpvPlayer::pause() {
    if (m_state == VIDEO_PLAYING)
        m_mpv.set_property_async("pause", true);
}

void MpvPlayer::stop() {
    const char *args[] = {"stop", nullptr};
    m_mpv.command_async(args);
}

void MpvPlayer::setSpeed(float speed) {
    speed = qBound(0.1f, speed, 10.0f);
    if (m_speed == speed) return;
    m_speed = speed;
    m_mpv.set_property_async("speed", static_cast<double>(speed));
    showText(QString("Speed: %1x").arg(speed));
    emit speedChanged();
    Settings::instance().set(Config::Speed, static_cast<double>(speed));
}

void MpvPlayer::seek(qint64 time, bool absolute) {
    if (m_state == STOPPED) return;
    if (absolute && time == m_time.load(std::memory_order_relaxed)) return;
    if (absolute && time < 0) time = 0;
    QByteArray time_str = QByteArray::number(time);
    const char *args[] = {"seek", time_str.constData(),
                          absolute ? "absolute" : "relative", nullptr};
    m_mpv.command_async(args);
}

void MpvPlayer::setVolume(int volume) {
    volume = qBound(0, volume, 200);
    if (m_volume == volume) return;
    m_volume = volume;
    m_mpv.set_property_async("volume", static_cast<double>(volume));
    showText(QString("Volume: %1%").arg(volume));
    emit volumeChanged();
    if (volume > 0)
        Settings::instance().set(Config::Volume, volume);
}

void MpvPlayer::setSubVisible(bool subVisible) {
    if (m_subVisible == subVisible) return;
    m_subVisible = subVisible;
    m_mpv.set_property_async("sub-visibility", m_subVisible);
    emit subVisibleChanged();
    if (!m_applyingTrackPrefs) saveTrackPrefs();
}

bool MpvPlayer::addVideo(const Track &video) {
    if (m_state == STOPPED) return false;
    if (!m_videoListModel.append(video.url, video.title, video.lang)) return true;
    QByteArray url = (video.url.isLocalFile() ? video.url.toLocalFile() : video.url.toString()).toUtf8();
    const char *args[] = {"video-add", url.constData(), "auto", "", nullptr};
    m_mpv.command_async(args);
    return true;
}

void MpvPlayer::sendKeyPress(const QString &key) {
    if (key.isEmpty() || key.endsWith('+')) return;
    QByteArray cmd = key.toUtf8();
    const char *args[] = {"keypress", cmd.constData(), nullptr};
    m_mpv.command_async(args);
}

bool MpvPlayer::addAudio(const Track &audio, bool select) {
    if (m_state == STOPPED) return false;
    if (!m_audioListModel.append(audio.url, audio.title, audio.lang)) return true;
    QByteArray url = (audio.url.isLocalFile() ? audio.url.toLocalFile() : audio.url.toString()).toUtf8();
    QByteArray title = audio.title.toUtf8();
    const char *mode = select ? "select" : "auto";
    const char *args[] = {"audio-add", url.constData(), mode, title.constData(), "", nullptr};
    m_mpv.command_async(args);
    return true;
}

bool MpvPlayer::addSubtitle(const Track &subtitle) {
    if (m_state == STOPPED) return false;
    if (!m_subtitleListModel.append(subtitle.url, subtitle.title, subtitle.lang)) return true;
    QByteArray url = (subtitle.url.isLocalFile() ? subtitle.url.toLocalFile() : subtitle.url.toString()).toUtf8();
    QByteArray title = subtitle.title.toUtf8();
    QByteArray lang = subtitle.lang.toUtf8();
    // Auto-select the English track so subs appear immediately.
    const QString l = subtitle.lang.toLower();
    const bool isEnglish = l == "en" || l == "eng" || l.startsWith("en-") || l.startsWith("en_")
                           || subtitle.title.contains("english", Qt::CaseInsensitive);
    const char *args[] = {"sub-add", url.constData(),
                          isEnglish ? "select" : "auto",
                          title.constData(), lang.constData(), nullptr};
    m_mpv.command_async(args);
    return true;
}

void MpvPlayer::screenshot() {
    if (m_state == STOPPED) return;
    const char *args[] = {"osd-msg", "screenshot", nullptr};
    m_mpv.command_async(args);
}

void MpvPlayer::onMpvEvent() {
    while (true) {
        const mpv_event *event = m_mpv.wait_event();
        if (!event || event->event_id == MPV_EVENT_NONE) break;
        switch (event->event_id) {
        case MPV_EVENT_START_FILE:      onStartFile(); break;
        case MPV_EVENT_FILE_LOADED:     onFileLoaded(); break;
        case MPV_EVENT_END_FILE:        onEndFile(event); break;
        case MPV_EVENT_IDLE:            onIdle(); break;
        case MPV_EVENT_VIDEO_RECONFIG:  onVideoReconfig(); break;
        case MPV_EVENT_LOG_MESSAGE:     onLogMessage(event); break;
        case MPV_EVENT_PROPERTY_CHANGE: onPropertyChange(event); break;
        default: break;
        }
    }
}

void MpvPlayer::onStartFile() {
    m_playNextEmitted = false;
    m_subRestored = false;      // re-apply per-show track prefs for this file
    m_videoPrefApplied = false;
    m_audioPrefApplied = false;
    m_videoWidth.store(0, std::memory_order_relaxed); m_videoHeight = 0;
    m_time.store(0, std::memory_order_relaxed);
    m_subVisible = true;
    emit timeChanged();
    emit subVisibleChanged();
}

void MpvPlayer::onFileLoaded() {
    m_state = VIDEO_PLAYING;
    DisplaySleep::inhibit();

    if (m_seekTime > 0) {
        seek(m_seekTime, true);
        m_seekTime = 0;
    }

    m_videoListModel.clear();
    m_videoResolutions.clear();
    if (!m_videosToBeAdded.isEmpty()) {
        for (const Video &v : std::as_const(m_videosToBeAdded))   // index-aligned with the model
            m_videoResolutions.append(v.resolution);
        m_videoListModel.append(m_videosToBeAdded[0].url,
                                m_videosToBeAdded[0].title,
                                m_videosToBeAdded[0].lang);
        for (int i = 1; i < m_videosToBeAdded.count(); i++)
            addVideo(m_videosToBeAdded[i]);
        m_videosToBeAdded.clear();
    }
    m_audioListModel.clear();
    for (int i = 0; i < m_audiosToBeAdded.size(); ++i)
        addAudio(m_audiosToBeAdded[i], /*select=*/i == 0);
    m_audiosToBeAdded.clear();

    m_subtitleListModel.clear();
    for (const auto &sub : std::as_const(m_subtitlesToBeAdded))
        addSubtitle(sub);
    m_subtitlesToBeAdded.clear();

    setLoading(false);
    emit mpvStateChanged();
}

void MpvPlayer::onEndFile(const mpv_event *event) {
    auto *ef = static_cast<mpv_event_end_file *>(event->data);
    handleMpvError(ef->error);
    m_endFileReason = static_cast<mpv_end_file_reason>(ef->reason);
    setLoading(false);
    if (m_endFileReason == MPV_END_FILE_REASON_ERROR)
        emit playbackError();   // drives auto-fallback to the next working server
    // Files with no duration never reach the time-based check below, so advance on real EOF.
    else if (m_endFileReason == MPV_END_FILE_REASON_EOF && !m_playNextEmitted) {
        m_playNextEmitted = true;
        emit playNext();
    }
}

void MpvPlayer::onIdle() {
    m_state = STOPPED;
    emit mpvStateChanged();
}

void MpvPlayer::onVideoReconfig() {
    Mpv::Node width = m_mpv.get_property("dwidth");
    Mpv::Node height = m_mpv.get_property("dheight");
    if (width.type() != MPV_FORMAT_NONE) {
        m_videoWidth.store(int(int64_t(width)), std::memory_order_relaxed);
        m_videoHeight = height;
        update();
        emit videoSizeChanged();
    }
}

void MpvPlayer::onLogMessage(const mpv_event *event) {
    if (!Settings::instance().mpvLogEnabled()) return;
    auto *msg = static_cast<mpv_event_log_message *>(event->data);
    static QString lastMsgText;
    auto msgText = QString::fromUtf8(msg->text).trimmed();
    if (!msgText.isEmpty() && msgText != lastMsgText) {
        lastMsgText = msgText;
        mLog() << "MPV" << msgText;
    }
}

void MpvPlayer::onPropertyChange(const mpv_event *event) {
    auto *prop = static_cast<mpv_event_property *>(event->data);
    if (prop->data == nullptr) return;

    const Mpv::Node &propValue = *static_cast<Mpv::Node *>(prop->data);
    if (propValue.type() == MPV_FORMAT_NONE) return;

    if (strcmp(prop->name, "playback-time") == 0) {
        int64_t newTime = static_cast<double>(propValue);
        if (newTime == m_time.load(std::memory_order_relaxed)) return;
        m_time.store(newTime, std::memory_order_relaxed);
        emit timeChanged();

        int64_t curTime = newTime;
        int64_t curDuration = m_duration.load(std::memory_order_relaxed);
        // Prefer AniSkip times + auto-skip; fall back to the manual times when AniSkip found nothing.
        const int64_t edLen = m_hasED ? m_aniEDLength : m_EDLength;
        const bool edWindow = edLen > 0 && edLen < curDuration && curTime > curDuration - edLen;
        // curDuration is 0 for live/duration-less streams, where this would fire on the first tick.
        if (!m_playNextEmitted && ((curDuration > 0 && curTime >= curDuration) ||
                (edWindow && (m_hasED ? Settings::instance().get(Config::AniSkipAuto) : m_skipED)))) {
            m_playNextEmitted = true;
            emit playNext();
        } else {
            const int64_t opStart = m_hasOP ? m_aniOPStart : m_OPStart;
            const int64_t opLen   = m_hasOP ? m_aniOPLength : m_OPLength;
            if (opLen > 0 && curTime >= opStart && curTime < opStart + opLen && opStart + opLen <= curDuration
                    && (m_hasOP ? Settings::instance().get(Config::AniSkipAuto) : m_skipOP)) {
                seek(opStart + opLen, true);
            }
        }
    }
    else if (strcmp(prop->name, "duration") == 0) {
        m_duration.store(static_cast<int64_t>(static_cast<double>(propValue)), std::memory_order_relaxed);
        emit durationChanged();
    }
    else if (strcmp(prop->name, "pause") == 0) {
        if (propValue && m_state == VIDEO_PLAYING) {
            m_state = VIDEO_PAUSED;
            DisplaySleep::allow();
        } else if (!propValue && m_state == VIDEO_PAUSED) {
            m_state = VIDEO_PLAYING;
            DisplaySleep::inhibit();
        }
        emit mpvStateChanged();
    }
    else if (strcmp(prop->name, "paused-for-cache") == 0) {
        if (propValue && m_state != STOPPED)
            showText("Network is slow...");
    }
    else if (strcmp(prop->name, "base-idle") == 0) {
        if (propValue && m_state == VIDEO_PLAYING)
            showText("Pausing...");
    }
    else if (strcmp(prop->name, "aid") == 0) {
        if (propValue.type() == MPV_FORMAT_INT64)
            m_audioListModel.setCurrentId(static_cast<int64_t>(propValue));
    }
    else if (strcmp(prop->name, "sid") == 0) {
        if (propValue.type() == MPV_FORMAT_INT64)
            m_subtitleListModel.setCurrentId(static_cast<int64_t>(propValue));
    }
    else if (strcmp(prop->name, "vid") == 0) {
        if (propValue.type() == MPV_FORMAT_INT64)
            m_videoListModel.setCurrentId(static_cast<int64_t>(propValue));
    }
    else if (strcmp(prop->name, "track-list") == 0) {
        parseTrackList(propValue);
    }
    else if (strcmp(prop->name, "glsl-shaders") == 0) {
        bool clamp = false;
        if (propValue.type() == MPV_FORMAT_NODE_ARRAY) {
            for (int i = 0; i < propValue.size() && !clamp; ++i) {
                const Mpv::Node &s = propValue[i];
                if (s.type() == MPV_FORMAT_STRING)
                    clamp = strstr(static_cast<const char *>(s), "AutoDownscalePre") != nullptr;
            }
        }
        if (clamp != m_bandClamp.load(std::memory_order_relaxed)) {
            m_bandClamp.store(clamp, std::memory_order_relaxed);
            update();
        }
    }
}

void MpvPlayer::parseTrackList(const Mpv::Node &trackList) {
    for (const Mpv::Node &track : trackList) {
        try {
            QString trackType = static_cast<const char *>(track["type"]);
            TrackListModel *listModel = nullptr;

            if (trackType == "sub")        listModel = &m_subtitleListModel;
            else if (trackType == "audio") listModel = &m_audioListModel;
            else if (trackType == "video") listModel = &m_videoListModel;
            else continue;

            int64_t id = -1;
            QString title, lang;
            QUrl url;
            double fps = 0.0;
            int64_t bitrate = 0, w = 0, h = 0;

            auto map = track.list();
            for (int i = 0; i < map->num; i++) {
                const char *key = map->keys[i];
                mpv_node &v = map->values[i];

                if (strcmp(key, "id") == 0 && v.format == MPV_FORMAT_INT64)
                    id = v.u.int64;
                else if (strcmp(key, "title") == 0 && v.format == MPV_FORMAT_STRING)
                    title = QString::fromUtf8(v.u.string);
                else if (strcmp(key, "lang") == 0 && v.format == MPV_FORMAT_STRING)
                    lang = QString::fromUtf8(v.u.string);
                else if (strcmp(key, "external-filename") == 0 && v.format == MPV_FORMAT_STRING)
                    url = QUrl(QString::fromUtf8(v.u.string));
                else if (strcmp(key, "demux-fps") == 0 && v.format == MPV_FORMAT_DOUBLE)
                    fps = v.u.double_;
                else if ((strcmp(key, "demux-bitrate") == 0 || strcmp(key, "hls-bitrate") == 0) && v.format == MPV_FORMAT_INT64) {
                    if (bitrate == 0) bitrate = v.u.int64;
                }
                else if (strcmp(key, "demux-w") == 0 && v.format == MPV_FORMAT_INT64)
                    w = v.u.int64;
                else if (strcmp(key, "demux-h") == 0 && v.format == MPV_FORMAT_INT64)
                    h = v.u.int64;
            }

            if (id <= 0) continue;

            // External file already handled by application
            if (!url.isEmpty())
                listModel->setId(url, id);

            listModel->setStats(id, static_cast<int>(h), fps, static_cast<int>(bitrate));

            if (listModel->indexForId(id) >= 0 && listModel->hasTitle(id))
                continue;

            QString label;
            if (trackType == "video") {
                QString resolution;
                if (w > 0 && h > 0) {
                    resolution = QString("%1x%2").arg(w).arg(h);
                    if (fps > 0)
                        resolution += QString(" %1FPS").arg(fps);
                }
                if (!title.isEmpty() && !resolution.isEmpty())
                    label = QString("%1 [%2]").arg(title, resolution);
                else
                    label = title.isEmpty() ? resolution : title;

                if (!label.isEmpty() && !lang.isEmpty())
                    label = QString("%1 (%2)").arg(label, lang);
                else if (label.isEmpty())
                    label = lang;
            } else {
                // Audio / subtitle
                if (!title.isEmpty() && !lang.isEmpty())
                    label = QString("%1 [%2]").arg(title, lang);
                else
                    label = title.isEmpty() ? lang : title;
            }

            if (bitrate > 0) {
                if (!label.isEmpty()) label += " - ";
                label += QString("%1 kbps").arg(bitrate / 1000);
            }

            if (label.isEmpty())
                label = QString("Track %1").arg(id);

            if (listModel->indexForId(id) >= 0)
                listModel->updateById(id, label);
            else
                listModel->append(id, label);

            listModel->setStats(id, static_cast<int>(h), fps, static_cast<int>(bitrate));

        } catch (const std::exception &e) {
            mLog() << "MPV" << e.what();
        }
    }
    m_videoListModel.sortByQuality(true);
    m_audioListModel.sortByQuality(false);
    m_videoResolutions = m_videoListModel.heights();

    // mpv may report vid/aid/sid before the tracks exist (and that pending id is lost on clear), so
    // re-sync from mpv now that the tracks are present.
    auto syncSelection = [this](const char *prop, TrackListModel &model) {
        Mpv::Node v = m_mpv.get_property(prop);
        if (v.type() == MPV_FORMAT_INT64) model.setCurrentId(static_cast<int64_t>(v));
    };
    syncSelection("vid", m_videoListModel);
    syncSelection("aid", m_audioListModel);
    syncSelection("sid", m_subtitleListModel);

    restoreTrackPrefs();   // re-apply the show's remembered audio/sub track once tracks load
}

void MpvPlayer::setProperty(const QString &name, const QVariant &value) {
    QByteArray nameData = name.toLatin1();
    switch (value.typeId()) {
    case QMetaType::Bool:
        m_mpv.set_property_async(nameData.constData(), value.toBool());
        break;
    case QMetaType::Int:
    case QMetaType::Long:
    case QMetaType::LongLong:
        m_mpv.set_property_async(nameData.constData(), value.toLongLong());
        break;
    case QMetaType::Float:
    case QMetaType::Double:
        m_mpv.set_property_async(nameData.constData(), value.toDouble());
        break;
    case QMetaType::QByteArray: {
        QByteArray v = value.toByteArray();
        m_mpv.set_property_async(nameData.constData(), v.constData());
        break;
    }
    case QMetaType::QString: {
        QByteArray v = value.toString().toUtf8();
        m_mpv.set_property_async(nameData.constData(), v.constData());
        break;
    }
    }
}

void MpvPlayer::handleMpvError(int code) {
    if (code < 0) {
        if (m_lastMpvError == code) {
            stop();
            m_lastMpvError = MPV_ERROR_SUCCESS;
            return;
        }
        m_lastMpvError = code;
        UiBridge::instance().showError(mpv_error_string(code),
                                       QString("Mpv Error %1").arg(code));
    }
}

void MpvPlayer::showText(const QString &text) {
    QByteArray data = text.toUtf8();
    const char *args[] = {"show-text", data.constData(), nullptr};
    m_mpv.command_async(args);
}

QQuickFramebufferObject::Renderer *MpvPlayer::createRenderer() const {
    QQuickWindow *win = window();
    Q_ASSERT(win != nullptr);
    win->setPersistentGraphics(true);
    win->setPersistentSceneGraph(true);
    return new MpvRenderer(const_cast<MpvPlayer *>(this));
}

void MpvPlayer::setHeaders(const QMap<QString, QString> &headers) {
    m_mpv.set_property("referrer", "");
    m_mpv.set_property("user-agent", "");
    m_mpv.set_property("http-header-fields", "");
    m_mpv.set_property("stream-lavf-o", "");
    if (headers.isEmpty()) return;

    QStringList extra;
    for (auto it = headers.constBegin(); it != headers.constEnd(); ++it) {
        const QString key = it.key().toLower();
        if (key == "user-agent")
            m_mpv.set_property("user-agent", it.value().toUtf8().constData());
        else if (key == "referrer" || key == "referer")
            m_mpv.set_property("referrer", it.value().toUtf8().constData());
        // Pass headers via http-header-fields too - the UA/referrer options miss ffmpeg's HLS segments.
        extra << QStringLiteral("%1: %2").arg(it.key(), it.value());
    }
    if (!extra.isEmpty())
        m_mpv.set_property("http-header-fields", extra.join(",").toUtf8().constData());
}

void MpvPlayer::setSkipOP(bool skip) {
    m_skipOP = skip;
    emit skipOPChanged();
}

void MpvPlayer::setSkipED(bool skip) {
    m_skipED = skip;
    emit skipEDChanged();
}

void MpvPlayer::setOPStart(qint64 start) {
    m_OPStart = start;
    emit skipOPStartChanged();
}

void MpvPlayer::setOPLength(qint64 length) {
    m_OPLength = length;
    emit skipOPLengthChanged();
}

void MpvPlayer::setEDLength(qint64 length) {
    m_EDLength = length;
    emit skipEDLengthChanged();
}

void MpvPlayer::setAudioIndex(int index) {
    if (index < 0 || index >= m_audioListModel.count()) return;
    m_mpv.set_property_async("aid", m_audioListModel.idForIndex(index));
    m_audioListModel.setCurrentIndex(index);
    if (!m_applyingTrackPrefs) { m_audioPrefApplied = true; saveTrackPrefs(); }  // user took control
}

void MpvPlayer::setSubIndex(int index) {
    if (index < 0 || index >= m_subtitleListModel.count()) return;
    m_mpv.set_property_async("sid", m_subtitleListModel.idForIndex(index));
    m_subtitleListModel.setCurrentIndex(index);
    if (!m_applyingTrackPrefs) { m_subRestored = true; saveTrackPrefs(); }
}

// Per-show track memory: save video as resolution+rank and audio as title+rank, re-applied later.
void MpvPlayer::saveTrackPrefs() {
    if (m_applyingTrackPrefs || m_showKey.isEmpty()) return;
    const Track *sub = m_subtitleListModel.at(m_subtitleListModel.getCurrentIndex());
    const Track *aud = m_audioListModel.at(m_audioListModel.getCurrentIndex());

    int vidIdx = m_videoListModel.getCurrentIndex();
    int vidRes = (vidIdx >= 0 && vidIdx < m_videoResolutions.size()) ? m_videoResolutions[vidIdx] : -1;
    int vidWithin = 0;
    for (int i = 0; i < vidIdx && i < m_videoResolutions.size(); ++i)
        if (m_videoResolutions[i] == vidRes) vidWithin++;

    const QStringList parts = {
        sub ? sub->title : QString(),
        aud ? aud->title : QString(),
        m_subVisible ? QStringLiteral("1") : QStringLiteral("0"),
        QString::number(vidRes),
        QString::number(vidWithin),
        QString::number(m_audioListModel.getCurrentIndex()),
    };
    Settings::instance().setString("tracks/" + m_showKey, parts.join(QChar(0x1f)));
}

// Map a saved (resolution, rank) onto the current list; exact res wins, else nearest.
int MpvPlayer::pickVideoForPrefs(int savedRes, int savedWithin) const {
    if (m_videoResolutions.isEmpty() || savedRes < 0) return -1;
    int start = -1, count = 0;
    for (int i = 0; i < m_videoResolutions.size(); ++i)
        if (m_videoResolutions[i] == savedRes) { if (start < 0) start = i; ++count; }
    if (start >= 0) return start + qMin(savedWithin, count - 1);

    int best = 0, bestDiff = std::numeric_limits<int>::max();
    for (int i = 0; i < m_videoResolutions.size(); ++i) {
        int d = qAbs(m_videoResolutions[i] - savedRes);
        if (d < bestDiff) { bestDiff = d; best = i; }
    }
    return best;
}

// Exact title match (language dubs) first, then the saved rank (bitrate variants).
int MpvPlayer::pickAudioForPrefs(const QString &savedTitle, int savedRank) {
    if (!savedTitle.isEmpty())
        for (int i = 0; i < m_audioListModel.count(); ++i)
            if (m_audioListModel.at(i)->title == savedTitle) return i;
    if (savedRank >= 0 && savedRank < m_audioListModel.count()) return savedRank;
    return -1;
}

// On every track-list update, nudge mpv toward the saved selection until it sticks.
void MpvPlayer::restoreTrackPrefs() {
    if (m_showKey.isEmpty() || (m_subRestored && m_videoPrefApplied && m_audioPrefApplied)) return;
    const QString pref = Settings::instance().getString("tracks/" + m_showKey);
    if (pref.isEmpty()) { m_subRestored = m_videoPrefApplied = m_audioPrefApplied = true; return; }
    const QStringList p = pref.split(QChar(0x1f));

    // Video quality (rank-based).
    if (!m_videoPrefApplied) {
        int target = (p.size() >= 5) ? pickVideoForPrefs(p[3].toInt(), p[4].toInt()) : -1;
        if (target < 0 || m_videoListModel.getCurrentIndex() == target) {
            m_videoPrefApplied = true;
        } else {
            int64_t id = m_videoListModel.idForIndex(target);
            if (id > 0) m_mpv.set_property_async("vid", id);
        }
    }

    // Audio track (title-or-rank).
    if (!m_audioPrefApplied) {
        int target = (p.size() >= 6) ? pickAudioForPrefs(p[1], p[5].toInt())
                   : (p.size() >= 2) ? pickAudioForPrefs(p[1], -1) : -1;
        if (target < 0 || m_audioListModel.getCurrentIndex() == target) {
            m_audioPrefApplied = true;
        } else {
            int64_t id = m_audioListModel.idForIndex(target);
            if (id > 0) m_mpv.set_property_async("aid", id);
        }
    }

    // Sub track + visibility (title match; subs load asynchronously too).
    if (!m_subRestored && p.size() >= 3) {
        m_applyingTrackPrefs = true;
        bool subFound = p[0].isEmpty();
        if (!p[0].isEmpty())
            for (int i = 0; i < m_subtitleListModel.count(); ++i)
                if (m_subtitleListModel.at(i)->title == p[0]) { setSubIndex(i); subFound = true; break; }
        setSubVisible(p[2] == "1");
        m_applyingTrackPrefs = false;
        if (subFound) m_subRestored = true;
    } else if (p.size() < 3) {
        m_subRestored = true;
    }
}

void MpvPlayer::setVideoIndex(int index) {
    if (index < 0 || index >= m_videoListModel.count()) return;
    m_mpv.set_property_async("vid", m_videoListModel.idForIndex(index));
    m_videoListModel.setCurrentIndex(index);
    auto *track = m_videoListModel.at(index);
    if (track && !track->url.isEmpty())
        m_currentVideoUrl = track->url;
    if (!m_applyingTrackPrefs) { m_videoPrefApplied = true; saveTrackPrefs(); }  // user took control
}

void MpvPlayer::setMuted(bool muted) {
    if (m_muted == muted) return;
    if (muted) {
        m_lastVolume = m_volume;
        setVolume(0);
    } else {
        setVolume(m_lastVolume);
    }
    m_muted = muted;
    emit mutedChanged();
}
