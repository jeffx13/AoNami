#include "mpvrenderworker.h"
#include "app/logger.h"
#include <QOpenGLContext>
#include <QOffscreenSurface>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QMutexLocker>

unsigned MpvRenderWorker::frontTexture() const {
    return m_fbo[m_front] ? m_fbo[m_front]->texture() : 0;
}

QSize MpvRenderWorker::frontSize() const {
    return m_fbo[m_front] ? m_fbo[m_front]->size() : QSize();
}

void MpvRenderWorker::initialize(QOpenGLContext *shareContext, QOffscreenSurface *surface) {
    m_surface = surface;
    // Share against Qt's global context (stable across threads; the scene-graph one silently fails).
    QOpenGLContext *share = QOpenGLContext::globalShareContext();
    if (!share) share = shareContext;
    if (!share) {
        rLog() << "Mpv" << "render worker: no share context available";
        return;
    }
    m_ctx = new QOpenGLContext;
    m_ctx->setShareContext(share);
    m_ctx->setFormat(share->format());
    if (!m_ctx->create() || !m_ctx->shareContext()) {
        rLog() << "Mpv" << "render worker: failed to create a shared GL context";
        delete m_ctx; m_ctx = nullptr;
        return;
    }
    if (!m_ctx->makeCurrent(m_surface)) {
        rLog() << "Mpv" << "render worker: makeCurrent failed";
        return;
    }

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
    if (m_mpv->renderer_initialize(params) < 0) {
        rLog() << "Mpv" << "render worker: mpv renderer_initialize failed";
        m_ctx->doneCurrent();
        return;
    }
    m_mpv->set_render_callback([](void *ctx) {
        QMetaObject::invokeMethod(static_cast<MpvRenderWorker *>(ctx), "renderFrame", Qt::QueuedConnection);
    }, this);
    m_inited = true;
    m_ctx->doneCurrent();
    QMetaObject::invokeMethod(this, "renderFrame", Qt::QueuedConnection);   // bootstrap first frame
}

void MpvRenderWorker::setSize(QSize sizePx) {
    { QMutexLocker lk(&m_mutex); m_pendingSize = sizePx; }
    QMetaObject::invokeMethod(this, "renderFrame", Qt::QueuedConnection);
}

void MpvRenderWorker::ensureFbos() {
    QSize want;
    { QMutexLocker lk(&m_mutex); want = m_pendingSize.isValid() ? m_pendingSize : m_size; }
    if (want.isEmpty()) return;
    if (m_fbo[0] && m_fbo[0]->size() == want) return;

    QOpenGLFramebufferObjectFormat fmt;
    fmt.setAttachment(QOpenGLFramebufferObject::NoAttachment);
    QMutexLocker lk(&m_mutex);   // QML thread may be reading m_fbo[m_front]
    for (auto &fbo : m_fbo) {
        delete fbo;
        fbo = new QOpenGLFramebufferObject(want, fmt);
    }
    m_size = want;
    m_hasFrame = false;
}

void MpvRenderWorker::renderFrame() {
    if (!m_inited || !m_ctx) return;
    if (!m_ctx->makeCurrent(m_surface)) return;

    ensureFbos();
    const int back = 1 - m_front;
    if (!m_fbo[back]) { m_ctx->doneCurrent(); return; }

    // Only render when mpv has a new frame (resize forces a repaint once).
    const uint64_t flags = m_mpv->render_update_flags();
    if (!(flags & MPV_RENDER_UPDATE_FRAME) && m_hasFrame) { m_ctx->doneCurrent(); return; }

    mpv_opengl_fbo mpfbo{static_cast<int>(m_fbo[back]->handle()),
                         m_fbo[back]->width(), m_fbo[back]->height(), 0};
    int flip_y = 0;
    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_OPENGL_FBO, &mpfbo},
        {MPV_RENDER_PARAM_FLIP_Y, &flip_y},
        {MPV_RENDER_PARAM_INVALID, nullptr}
    };
    m_mpv->render(params);                 // the expensive work - NOT under the mutex
    m_ctx->functions()->glFinish();        // ensure the texture is complete before QML samples it

    { QMutexLocker lk(&m_mutex); m_front = back; m_hasFrame = true; }
    m_ctx->doneCurrent();
    emit frameReady();
}

void MpvRenderWorker::shutdown() {
    if (m_ctx && m_ctx->makeCurrent(m_surface)) {
        m_mpv->free_renderer();            // must happen with this thread's GL context current
        delete m_fbo[0]; delete m_fbo[1];
        m_fbo[0] = m_fbo[1] = nullptr;
        m_ctx->doneCurrent();
    }
    delete m_ctx; m_ctx = nullptr;
    m_inited = false;
}
