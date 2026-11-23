#pragma once
#include <QObject>
#include <QString>
#include <QByteArray>
#include <QList>
#include <vector>
#include <alsa/asoundlib.h>

class QSocketNotifier;
class QTimer;

// One USB webcam (MJPEG video + optional ALSA mic). Hotplug-safe: after
// begin() it retries every second, so unplug/replug resumes automatically.
// Create one instance per camera, move it to a thread, and connect that
// thread's started() signal to begin().
class Webcam : public QObject
{
    Q_OBJECT
public:
    // videoDev: prefer /dev/v4l/by-path/...-video-index0 (stable per USB port)
    // audioDev: ALSA device string, or empty for video only
    Webcam(const QString &name, const QString &videoDev,
           const QString &audioDev = QString(), QObject *parent = nullptr);
    ~Webcam() override;

public slots:
    void begin();   // call on the owning thread; starts the retry timer

    signals:
        void frameReady(const QString &name, const QByteArray &jpeg, quint64 tsNs);
    void audioReady(const QString &name, const QByteArray &pcm, quint64 tsNs,
                    quint32 rate, quint32 channels);

private slots:
    void scan();
    void onReadable();
    void onAudioReadable();

private:
    bool start();
    void stop();
    bool startAudio();
    void stopAudio();

    struct Buf { void *start; size_t length; };

    QString m_name, m_videoDev, m_audioDev;
    int m_width = 640, m_height = 480, m_fps = 30;
    unsigned m_audioRate = 48000, m_audioChannels = 1;

    QTimer *m_timer = nullptr;

    int m_fd = -1;
    std::vector<Buf> m_bufs;
    QSocketNotifier *m_notifier = nullptr;

    snd_pcm_t *m_pcm = nullptr;
    snd_pcm_uframes_t m_period = 0;
    unsigned m_rate = 0, m_channels = 0;
    QByteArray m_audioBuf;
    QList<QSocketNotifier *> m_audioNotifiers;
};