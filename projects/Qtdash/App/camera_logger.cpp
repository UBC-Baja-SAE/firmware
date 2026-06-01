#include "camera_logger.h"
#include "mcap_logger.h"

#include <QVideoFrame>
#include <QImage>
#include <QBuffer>
#include <QByteArray>
#include <QMediaDevices>
#include <QDebug>
#include <chrono>

static uint64_t nowNs() {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count());
}

CameraLogger::CameraLogger(QObject* parent) : QObject(parent) {}

void CameraLogger::start() {
    auto cameras = QMediaDevices::videoInputs();
    if (cameras.isEmpty()) {
        qWarning() << "CameraLogger: No cameras found";
        return;
    }

    session_ = new QMediaCaptureSession(this);
    camera_  = new QCamera(this);
    sink_    = new QVideoSink(this);

    camera_->setCameraDevice(cameras.first());
    qDebug() << "CameraLogger: Using" << cameras.first().description();

    session_->setCamera(camera_);
    session_->setVideoSink(sink_);

    // Single sink feeds both the MCAP logger and, via frameReady signal,
    // any QML VideoOutput that connects its videoSink to us.
    connect(sink_, &QVideoSink::videoFrameChanged,
            this,  &CameraLogger::onFrameReady);

    camera_->start();
    qDebug() << "CameraLogger: started";
}

void CameraLogger::stop() {
    if (camera_) {
        camera_->stop();
        qDebug() << "CameraLogger: stopped";
    }
}

void CameraLogger::onFrameReady(const QVideoFrame& frame) {
    if (!frame.isValid()) return;

    // Broadcast raw frame to QML preview before any heavy JPEG work.
    emit frameReady(frame);

    // Drop frames silently until the MCAP thread has finished channel setup.
    if (!McapLogger::instance().isCameraChannelReady()) return;

    QImage img = frame.toImage().convertToFormat(QImage::Format_RGB888);
    if (img.isNull()) return;

    QByteArray jpegData;
    QBuffer    buf(&jpegData);
    buf.open(QIODevice::WriteOnly);
    img.save(&buf, "JPEG", 70);
    buf.close();

    if (jpegData.isEmpty()) return;

    McapLogger::instance().logCameraFrame(
        reinterpret_cast<const uint8_t*>(jpegData.constData()),
        static_cast<size_t>(jpegData.size()),
        nowNs(),
        img.width(),
        img.height());
}