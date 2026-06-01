#pragma once
#include <QObject>
#include <QVideoSink>
#include <QVideoFrame>
#include <QCamera>
#include <QMediaCaptureSession>

class CameraLogger : public QObject {
    Q_OBJECT
public:
    explicit CameraLogger(QObject* parent = nullptr);
    void start();
    void stop();

signals:
    // Connect this in QML to a VideoOutput's videoSink property
    // so the preview and MCAP logger share one capture session.
    void frameReady(const QVideoFrame& frame);

private slots:
    void onFrameReady(const QVideoFrame& frame);

private:
    QCamera*              camera_   = nullptr;
    QVideoSink*           sink_     = nullptr;   // single sink — avoids dual-sink conflict
    QMediaCaptureSession* session_  = nullptr;
};