#ifndef WEBCAM_H
#define WEBCAM_H

#include <QObject>
#include <QImage>
#include <QString>
#include <QByteArray>

// Only include heavy multimedia headers on the release target
#ifdef ENV_RELEASE
#include <QCamera>
#include <QMediaCaptureSession>
#include <QVideoSink>
#include <QVideoFrame>
#include <QTimer>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QAudioFormat>
#include <QAudioSource>
#endif

class Webcam : public QObject {
    Q_OBJECT

public:
    explicit Webcam(QObject* parent = nullptr);

    Q_INVOKABLE void start();
    Q_INVOKABLE void setQmlVideoOutput(QObject* qmlOutput);

    signals:
        void frameReady(const QString& topic, const QImage& image);
    void audioReady(const QString& topic, const QByteArray& data, int sampleRate, int channels);

#ifdef ENV_RELEASE
private slots:
    void processFrame(const QVideoFrame& frame);
    void processAudio();
    void startAudio();

private:
    // Video Members
    QCamera* m_camera = nullptr;
    QMediaCaptureSession* m_session = nullptr;
    QVideoSink* m_videoSink = nullptr;
    QTimer m_throttleTimer;
    bool m_readyForNextFrame = false;

    // Audio Members
    QAudioSource* m_audioSource = nullptr;
    QIODevice* m_ioDevice = nullptr;
    int m_sampleRate = 44100;
    int m_channels = 1;
#endif
};

#endif // WEBCAM_H