#pragma once
#include <QObject>
#include <QMutex>
#include <QSize>
#include "mpv.hpp"

class QOpenGLContext;
class QOffscreenSurface;
class QOpenGLFramebufferObject;

// Renders mpv on a dedicated thread into double-buffered FBOs; the QML thread only
// blits the finished front texture (mutex-guarded), never blocking on mpv.
class MpvRenderWorker : public QObject {
    Q_OBJECT
public:
    explicit MpvRenderWorker(Mpv::Handle *mpv) : m_mpv(mpv) {}
    ~MpvRenderWorker() override = default;

    QMutex &mutex() { return m_mutex; }                 // guards the front index + FBO lifetime
    unsigned frontTexture() const;                      // GL texture id of the latest complete frame
    QSize    frontSize() const;
    bool     hasFrame() const { return m_hasFrame; }

public slots:
    void initialize(QOpenGLContext *shareContext, QOffscreenSurface *surface);
    void setSize(QSize sizePx);
    void renderFrame();   // triggered by mpv's render-update callback (and resize/bootstrap)
    void shutdown();

signals:
    void frameReady();    // a new front frame is ready -> MpvPlayer::update()

private:
    void ensureFbos();    // worker thread, context current

    Mpv::Handle *m_mpv;
    QOpenGLContext   *m_ctx     = nullptr;
    QOffscreenSurface *m_surface = nullptr;
    QOpenGLFramebufferObject *m_fbo[2] = {nullptr, nullptr};
    int   m_front   = 0;          // index the QML thread blits from
    QSize m_size;                 // current FBO size
    QSize m_pendingSize;          // requested size (from setSize)
    QMutex m_mutex;
    bool  m_inited   = false;
    bool  m_hasFrame = false;
};
