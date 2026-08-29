#pragma once

#include "media/mpv.h"
#include <QByteArray>
#include <QFutureWatcher>
#include <QQuickWindow>
#include <QtQuick/QQuickFramebufferObject>
#include <QTimer>
#include "media/playinfo.h"
#include "ui/models/tracklistmodel.h"
#include <atomic>

class MpvRenderer;

class MpvPlayer : public QQuickFramebufferObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(State             state          READ state                               NOTIFY mpvStateChanged)
    Q_PROPERTY(qint64            duration       READ duration                            NOTIFY durationChanged)
    Q_PROPERTY(qint64            time           READ time                                NOTIFY timeChanged)
    Q_PROPERTY(QSize             videoSize      READ videoSize                           NOTIFY videoSizeChanged)
    Q_PROPERTY(bool              subVisible     READ subVisible     WRITE setSubVisible  NOTIFY subVisibleChanged)
    Q_PROPERTY(double            subDelay       READ subDelay       WRITE setSubDelay    NOTIFY subDelayChanged)
    Q_PROPERTY(bool              skipOP         READ skipOP         WRITE setSkipOP      NOTIFY skipOPChanged)
    Q_PROPERTY(bool              skipED         READ skipED         WRITE setSkipED      NOTIFY skipEDChanged)
    Q_PROPERTY(qint64            skipOPStart    READ skipOPStart    WRITE setOPStart     NOTIFY skipOPStartChanged)
    Q_PROPERTY(qint64            skipOPLength   READ skipOPLength   WRITE setOPLength    NOTIFY skipOPLengthChanged)
    Q_PROPERTY(qint64            skipEDLength   READ skipEDLength   WRITE setEDLength    NOTIFY skipEDLengthChanged)
    Q_PROPERTY(bool              hasOP          READ hasOP                               NOTIFY hasOPChanged)
    Q_PROPERTY(bool              hasED          READ hasED                               NOTIFY hasEDChanged)
    Q_PROPERTY(qint64            aniOPStart     READ aniOPStart                          NOTIFY aniOPStartChanged)
    Q_PROPERTY(qint64            aniOPLength    READ aniOPLength                         NOTIFY aniOPLengthChanged)
    Q_PROPERTY(qint64            aniEDLength    READ aniEDLength                         NOTIFY aniEDLengthChanged)
    Q_PROPERTY(int               volume         READ volume         WRITE setVolume      NOTIFY volumeChanged)
    Q_PROPERTY(float             speed          READ speed          WRITE setSpeed       NOTIFY speedChanged)
    Q_PROPERTY(bool              muted          READ muted          WRITE setMuted       NOTIFY mutedChanged)
    Q_PROPERTY(bool              isLoading      READ isLoading                           NOTIFY isLoadingChanged)
    Q_PROPERTY(TrackListModel*   subtitleList   READ getSubtitleList CONSTANT)
    Q_PROPERTY(TrackListModel*   videoList      READ getVideoList    CONSTANT)
    Q_PROPERTY(TrackListModel*   audioList      READ getAudioList    CONSTANT)

    friend class MpvRenderer;
public:
    enum State { STOPPED, VIDEO_PLAYING, VIDEO_PAUSED, TV_PLAYING };
    Q_ENUM(State)

    inline static MpvPlayer *instance() { return s_instance.load(std::memory_order_acquire); }

    MpvPlayer(QQuickItem *parent = nullptr);
    ~MpvPlayer() override;
    virtual Renderer *createRenderer() const;

public:
    bool ensureMpvRenderContext();
    void freeMpvRenderContext();
    Mpv::Handle &handle() { return m_mpv; }
    QSize clampRenderSize(QSize itemPx) const;

    State state()       const { return m_state;      }
    qint64 duration()   const { return m_duration.load(std::memory_order_relaxed);   }
    qint64 time()       const { return m_time.load(std::memory_order_relaxed);       }
    bool muted()        const { return m_muted;      }
    bool subVisible()   const { return m_subVisible; }
    double subDelay()   const { return m_subDelay;   }
    int volume()        const { return m_volume;     }
    float speed()       const { return m_speed;      }
    bool skipOP()       const { return m_skipOP;     }
    bool skipED()       const { return m_skipED;     }
    qint64 skipOPStart()    const { return m_OPStart;    }
    qint64 skipOPLength()   const { return m_OPLength;   }
    qint64 skipEDLength()   const { return m_EDLength;   }
    bool hasOP()        const { return m_hasOP;      }
    bool hasED()        const { return m_hasED;      }
    qint64 aniOPStart()  const { return m_aniOPStart;  }
    qint64 aniOPLength() const { return m_aniOPLength; }
    qint64 aniEDLength() const { return m_aniEDLength; }
    bool isLoading()    const { return m_isLoading;  }
    QSize videoSize()   const { auto *w = window(); return QSize(m_videoWidth.load(std::memory_order_relaxed), m_videoHeight) / (w ? w->effectiveDevicePixelRatio() : 1.0); }

    void open(PlayInfo &playItem);
    Q_INVOKABLE void play(void);
    Q_INVOKABLE void pause(void);
    Q_INVOKABLE void stop(void);
    Q_INVOKABLE void seek(qint64 offset, bool absolute = true);
    Q_INVOKABLE void setSpeed(float speed);
    Q_INVOKABLE void setVolume(int volume);
    Q_INVOKABLE void setSubVisible(bool subVisible);
    Q_INVOKABLE void setSubDelay(double seconds);
    Q_INVOKABLE void screenshot(void);
    Q_INVOKABLE void setProperty(const QString &name, const QVariant &value);
    Q_INVOKABLE void showText(const QString &text);

    Q_INVOKABLE void setVideoIndex(int index);
    Q_INVOKABLE void setAudioIndex(int index);
    Q_INVOKABLE void setSubIndex(int index);
    Q_INVOKABLE void toggleSubIndex(int index);
    Q_INVOKABLE void setSecondarySubIndex(int index);
    Q_INVOKABLE void setSubPos(int pos);
    void setOPStart(qint64 start);
    void setOPLength(qint64 length);
    void setEDLength(qint64 length);
    void setHasOP(bool v) { if (m_hasOP == v) return; m_hasOP = v; emit hasOPChanged(); }
    void setHasED(bool v) { if (m_hasED == v) return; m_hasED = v; emit hasEDChanged(); }
    void setAniOPStart(qint64 v)  { if (m_aniOPStart == v) return;  m_aniOPStart = v;  emit aniOPStartChanged(); }
    void setAniOPLength(qint64 v) { if (m_aniOPLength == v) return; m_aniOPLength = v; emit aniOPLengthChanged(); }
    void setAniEDLength(qint64 v) { if (m_aniEDLength == v) return; m_aniEDLength = v; emit aniEDLengthChanged(); }

    Q_INVOKABLE QUrl getCurrentVideoUrl() const { return m_currentVideoUrl; }
    Q_INVOKABLE void sendKeyPress(const QString &key);

    void refreshDanmaku();   // re-render in place, no episode reload

    bool addVideo(const Track &video);
    bool addAudio(const Track &audio, bool select = false);
    bool addSubtitle(const Track &subtitle);
    void setHeaders(const QMap<QString, QString> &headers);
    void setShowKey(const QString &key) { m_showKey = key; }   // per-show sub/audio memory
    void setSkipOP(bool skip);
    void setSkipED(bool skip);
    void setMuted(bool muted);

signals:
    void durationChanged(void);
    void timeChanged(void);
    void volumeChanged(void);
    void speedChanged(void);
    void videoSizeChanged(void);
    void playNext(void);
    void playbackError(void);   // file failed to load/play (not a normal EOF/stop)
    void skipOPChanged(void);
    void skipEDChanged(void);
    void skipOPStartChanged(void);
    void skipOPLengthChanged(void);
    void skipEDLengthChanged(void);
    void hasOPChanged(void);
    void hasEDChanged(void);
    void aniOPStartChanged(void);
    void aniOPLengthChanged(void);
    void aniEDLengthChanged(void);
    void mpvStateChanged(void);
    void subVisibleChanged(void);
    void subDelayChanged(void);
    void isLoadingChanged(void);
    void mutedChanged();

private:
    Mpv::Handle m_mpv;
    inline static std::atomic<MpvPlayer *> s_instance{nullptr};
    State m_state = STOPPED;
    mpv_end_file_reason m_endFileReason = MPV_END_FILE_REASON_STOP;

    // Written on the GUI thread, read by clampRenderSize() on the render thread.
    std::atomic<QSize> m_maxRenderSize{QSize(1920, 1080)};
    std::atomic<bool>  m_bandClamp{false};                   // an Anime4K AutoDownscalePre chain is loaded
    bool  m_renderCtxInited = false;

    void recomputeMaxRenderSize();

    void setLoading(bool loading) {
        if (m_isLoading == loading) return;
        m_isLoading = loading;
        emit isLoadingChanged();
    }

    // Thread-safe: read from worker threads via time()/duration().
    std::atomic<int64_t> m_time{0};
    std::atomic<int64_t> m_duration{0};

    std::atomic<int> m_videoWidth{0};
    int64_t m_videoHeight = 0;
    QList<Track> m_audiosToBeAdded;
    QList<Track> m_subtitlesToBeAdded;
    QList<Video> m_videosToBeAdded;
    double m_seekFraction = 0.0;
    void   applyPendingSeek();
    QUrl m_currentVideoUrl;

    QList<DanmakuComment> m_danmaku;
    QString m_danmakuKey;
    QTimer  m_danmakuRefresh;   // coalesces slider drags

    TrackListModel m_subtitleListModel;
    TrackListModel m_audioListModel;
    TrackListModel m_videoListModel;

    double m_subDelay = 0.0;
    bool m_subVisible = false;
    float m_speed     = 1.0;
    int m_volume      = 100;
    int m_lastVolume  = 0;
    bool m_isLoading  = false;
    bool m_muted      = false;
    bool m_playNextEmitted = false;
    bool m_skipOP     = false;
    bool m_skipED     = false;
    int m_lastMpvError = MPV_ERROR_SUCCESS;
    qint64 m_OPStart  = 0;
    qint64 m_OPLength = 120;
    qint64 m_EDLength = 60;
    bool   m_hasOP    = false;
    bool   m_hasED    = false;
    qint64 m_aniOPStart  = 0;
    qint64 m_aniOPLength = 0;
    qint64 m_aniEDLength = 0;

    QString m_showKey;
    bool    m_subRestored        = false;
    bool    m_videoPrefApplied   = false;
    bool    m_audioPrefApplied   = false;
    bool    m_applyingTrackPrefs = false;   // suppress saving while restoring
    QList<int> m_videoResolutions;          // resolution per video-model index (sorted desc)
    void saveTrackPrefs();
    void restoreTrackPrefs();
    int  pickVideoForPrefs(int savedRes, int savedWithin) const;
    int  pickAudioForPrefs(const QString &savedTitle, int savedRank);

    TrackListModel *getSubtitleList() { return &m_subtitleListModel; }
    TrackListModel *getAudioList()    { return &m_audioListModel;    }
    TrackListModel *getVideoList()    { return &m_videoListModel;    }

    Q_INVOKABLE void onMpvEvent();
    void onStartFile();
    void onFileLoaded();
    void onEndFile(const mpv_event *event);
    void onIdle();
    void onVideoReconfig();
    void onLogMessage(const mpv_event *event);
    void onPropertyChange(const mpv_event *event);
    void parseTrackList(const Mpv::Node &trackList);
    int  danmakuTrackIndex() const;
    void setSubIndexInternal(int index);
    void applySubLayout();
    int    m_subPos = 100;
    int    m_secondarySubLines = 0;
    double m_subScale = 1.0;
    QFutureWatcher<QString> m_danmakuWriter;
    void handleMpvError(int code);
};