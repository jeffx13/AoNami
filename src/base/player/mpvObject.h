#pragma once

#include "mpv.hpp"
#include <QByteArray>
#include <QClipboard>
#include <QGuiApplication>
#include <QQuickWindow>
#include <QtQuick/QQuickFramebufferObject>
#include "base/player/playinfo.h"
#include "gui/tracklistmodel.h"

class MpvRenderer;

class MpvObject : public QQuickFramebufferObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(State state             READ state                              NOTIFY mpvStateChanged)
    Q_PROPERTY(qint64 duration         READ duration                           NOTIFY durationChanged)
    Q_PROPERTY(qint64 time             READ time                               NOTIFY timeChanged)
    Q_PROPERTY(QSize videoSize         READ videoSize                          NOTIFY videoSizeChanged)
    Q_PROPERTY(bool isLoading          READ isLoading                          NOTIFY isLoadingChanged)
    Q_PROPERTY(bool subVisible         READ subVisible   WRITE setSubVisible   NOTIFY subVisibleChanged)
    Q_PROPERTY(bool shouldSkipOP       READ shouldSkipOP WRITE setShouldSkipOP NOTIFY shouldSkipOPChanged)
    Q_PROPERTY(bool shouldSkipED       READ shouldSkipED WRITE setShouldSkipED NOTIFY shouldSkipEDChanged)
    Q_PROPERTY(int volume              READ volume       WRITE setVolume       NOTIFY volumeChanged)
    Q_PROPERTY(float speed             READ speed        WRITE setSpeed        NOTIFY speedChanged)
    Q_PROPERTY(TrackListModel *subtitleList READ getSubtitleList CONSTANT)
    Q_PROPERTY(TrackListModel *videoList READ getVideoList CONSTANT)
    Q_PROPERTY(TrackListModel *audioList READ getAudioList CONSTANT)


    friend class MpvRenderer;
public:
    enum State { STOPPED, VIDEO_PLAYING, VIDEO_PAUSED, TV_PLAYING };
    enum Hwdec { AUTO, VAAPI, VDPAU, NVDEC };
    Q_ENUM(State)

    inline static MpvObject *instance() { return s_instance; }

    MpvObject(QQuickItem *parent = nullptr);
    virtual Renderer *createRenderer() const;

    // Access properties
    inline State state()             const { return m_state; }
    inline qint64 duration()         const { return m_duration; }
    inline qint64 time()             const { return m_time; }
    inline bool subVisible()         const { return m_subVisible; }
    inline int volume()              const { return m_volume; }
    inline float speed()             const { return m_speed; }
    inline bool shouldSkipOP()              const { return m_shouldSkipOP; }
    inline bool shouldSkipED()              const { return m_shouldSkipED; }
    inline bool isResizing()                const { return m_isResizing; }
    inline bool isLoading()                 const { return m_isLoading; }
    inline QSize videoSize() const {
        return QSize(m_videoWidth, m_videoHeight) / window()->effectiveDevicePixelRatio();
    }

    // Methods
    Q_INVOKABLE void open(const PlayItem &playItem);
    Q_INVOKABLE void play(void);
    Q_INVOKABLE void pause(void);
    Q_INVOKABLE void stop(void);
    Q_INVOKABLE void mute(void);
    Q_INVOKABLE void setSpeed(float speed);
    Q_INVOKABLE void seek(qint64 offset, bool absolute = true);
    Q_INVOKABLE void screenshot(void);
    Q_INVOKABLE bool addAudio(const Track &audio);
    Q_INVOKABLE bool addSubtitle(const Track &subtitle);
    Q_INVOKABLE void setProperty(const QString &name, const QVariant &value);
    Q_INVOKABLE void showText(const QString &text);
    Q_INVOKABLE void setVolume(int volume);
    Q_INVOKABLE void setSubVisible(bool subVisible);
    Q_INVOKABLE void setIsResizing(bool isResizing) {
        m_isResizing = isResizing;
        if (!m_isResizing)
            update();
    }

    Q_INVOKABLE void setSkipTimeOP(int start, int length) {
        m_OPStart = start;
        m_OPEnd = start + length;
    }
    Q_INVOKABLE void setSkipTimeED(int start, int length) {
        m_EDStart = m_duration - length;
        m_EDEnd = m_duration;
    }
    Q_INVOKABLE QUrl getCurrentVideoUrl() const { return m_currentVideoUrl; }
    Q_INVOKABLE void sendKeyPress(QString key) {
        auto cmd = key.toStdString();
        const char *args[] = {"keypress", cmd.data(), nullptr};
        m_mpv.command_async(args);
    }

    void setHeaders(const QMap<QString, QString> &headers);

    Q_INVOKABLE void setAudioIndex(int index) {
        if (index < 0 || index >= m_audioListModel.count()) {
            qDebug() << "Invalid aid:" << index + 1;
            return;
        }
        m_mpv.set_property_async("aid", m_audioListModel.getId(index));
        m_audioListModel.setCurrentIndex(index);
    }
    Q_INVOKABLE void setSubIndex(int index) {
        if (index < 0 || index >= m_subtitleListModel.count()) {
            qDebug() << "Invalid sid:" << index + 1;
            return;
        }
        m_mpv.set_property_async("sid", m_subtitleListModel.getId(index));
        m_subtitleListModel.setCurrentIndex(index);
    }
    Q_INVOKABLE void setVideoIndex(int index) {
        if (index < 0 || index >= m_videoListModel.count()) {
            qDebug() << "Invalid vid:" << index;
            return;
        }
        m_mpv.set_property_async("vid", m_videoListModel.getId(index));
        m_videoListModel.setCurrentIndex(index);
    }


    TrackListModel *getSubtitleList() { return &m_subtitleListModel; }
    TrackListModel *getAudioList() { return &m_audioListModel; }
    TrackListModel *getVideoList() { return &m_videoListModel; }


    void setShouldSkipOP(bool skip){
        m_shouldSkipOP = skip;
        emit shouldSkipOPChanged();
    }
    void setShouldSkipED(bool skip){
        m_shouldSkipED = skip;
        emit shouldSkipEDChanged();
    }
signals:
    void durationChanged(void);
    void timeChanged(void);
    void volumeChanged(void);
    void speedChanged(void);
    void videoSizeChanged(void);
    void playNext(void);
    void shouldSkipOPChanged(void);
    void shouldSkipEDChanged(void);
    void audioTracksChanged(void);
    void mpvStateChanged(void);
    void subtitlesChanged(void);
    void subVisibleChanged(void);
    void isLoadingChanged(void);
    void errorCallback(int code);
private:
    Q_INVOKABLE void onMpvEvent(void);
    void handleMpvError(int code);

    Mpv::Handle m_mpv;
    inline static MpvObject *s_instance = nullptr;

    State m_state = STOPPED;
    mpv_end_file_reason m_endFileReason = MPV_END_FILE_REASON_STOP;

    int64_t m_time;
    int64_t m_duration;
    int64_t m_videoWidth = 0;
    int64_t m_videoHeight = 0;
    QList<Track> m_audioToBeAdded;
    QList<Track> m_subtitleToBeAdded;
    QList<Video> m_videosToBeAdded;

    TrackListModel m_subtitleListModel;
    TrackListModel m_audioListModel;
    TrackListModel m_videoListModel;

    bool m_subVisible = false;
    float m_speed = 1.0;
    int m_volume = 50;
    int m_lastVolume = 0;
    bool m_isLoading = false;

    bool m_shouldSkipOP = false;
    bool m_shouldSkipED = false;
    qint64 m_OPStart = 0;
    qint64 m_OPEnd = 90;
    qint64 m_EDStart = 0;
    qint64 m_EDEnd = 90;
    qint64 m_seekTime = 0;
    bool m_isResizing = false;

    QUrl m_currentVideoUrl;

};


