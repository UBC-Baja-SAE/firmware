#pragma once

#include <QObject>
#include <QCamera>
#include <QMediaCaptureSession>
#include <QVideoSink>
#include <QVideoFrame>
#include <QImage>
#include <QTimer>
#include <QVariant> // Needed to extract the video sink from QML

class Webcam : public QObject {
    Q_OBJECT
public:
    explicit Webcam(QObject* parent = nullptr);
    void start();

    // NEW: Method to link the C++ camera to the QML UI
    void setQmlVideoOutput(QObject* qmlOutput);

    signals:
        void frameReady(const QString& topic, const QImage& image);

private slots:
    void processFrame(const QVideoFrame& frame);

private:
    QCamera* m_camera;
    QMediaCaptureSession* m_session;
    QVideoSink* m_videoSink;
    QTimer m_throttleTimer;
    bool m_readyForNextFrame = true;
};