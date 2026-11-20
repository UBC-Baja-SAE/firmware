#pragma once
#include <QObject>
#include <QByteArray>
#include <QString>
#include <vector>

class QSocketNotifier;
struct _snd_pcm;

class Webcam : public QObject
{
    Q_OBJECT
public:
    explicit Webcam(QObject *parent = nullptr);
    ~Webcam() override;

public slots:
    bool start(const QString &device, int width, int height, int fps);
    void stop();
    bool startAudio(const QString &device, unsigned rate, unsigned channels);
    void stopAudio();

    signals:
        void frameReady(const QByteArray &jpeg, quint64 tsNs);
    void audioReady(const QByteArray &pcm, quint64 tsNs, quint32 rate, quint32 channels);

private:
    void onReadable();
    void onAudioReadable();

    struct Buf { void *start = nullptr; size_t length = 0; };
    int m_fd = -1;
    std::vector<Buf> m_bufs;
    QSocketNotifier *m_notifier = nullptr;

    _snd_pcm *m_pcm = nullptr;
    std::vector<QSocketNotifier*> m_audioNotifiers;
    unsigned m_rate = 0, m_channels = 0;
    unsigned long m_period = 0;
    QByteArray m_audioBuf;
};