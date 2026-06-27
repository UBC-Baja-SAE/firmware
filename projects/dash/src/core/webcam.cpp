#include "webcam.h"
#include <QDebug>

Webcam::Webcam(QObject* parent) : QObject(parent) {
    m_camera = new QCamera(this);
    m_session = new QMediaCaptureSession(this);
    m_videoSink = new QVideoSink(this);

    m_session->setCamera(m_camera);
    m_session->setVideoSink(m_videoSink);

    // Throttle frames to 10 FPS to prevent overwhelming the CPU for Foxglove
    connect(&m_throttleTimer, &QTimer::timeout, this, [this]() {
        m_readyForNextFrame = true;
    });
    m_throttleTimer.start(100);

    connect(m_videoSink, &QVideoSink::videoFrameChanged, this, &Webcam::processFrame);
}

void Webcam::start() {
    m_camera->start();
    qInfo() << "[Webcam] Started capture on default device";
}

void Webcam::setQmlVideoOutput(QObject* qmlOutput) {
    if (!qmlOutput) return;

    // Route the video session to render directly to the QML UI
    m_session->setVideoOutput(qmlOutput);

    // Grab the QVideoSink that the QML VideoOutput uses internally
    QVideoSink* qmlSink = qvariant_cast<QVideoSink*>(qmlOutput->property("videoSink"));
    if (qmlSink) {
        // Disconnect our invisible default sink and listen to the QML sink instead
        disconnect(m_videoSink, &QVideoSink::videoFrameChanged, this, &Webcam::processFrame);
        connect(qmlSink, &QVideoSink::videoFrameChanged, this, &Webcam::processFrame);
        qInfo() << "[Webcam] Routed video frames to QML UI";
    }
}

void Webcam::processFrame(const QVideoFrame& frame) {
    if (!m_readyForNextFrame || !frame.isValid()) return;
    
    m_readyForNextFrame = false; // Lock until timer resets
    
    QImage image = frame.toImage();
    if (!image.isNull()) {
        // Send scaled down image to Foxglove (C270 is 720p natively, scale to save CPU)
        QImage scaled = image.scaled(640, 480, Qt::KeepAspectRatio, Qt::FastTransformation);
        emit frameReady("/camera/front", scaled);
    }
}