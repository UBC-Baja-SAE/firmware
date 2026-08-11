#include "webcam.h"
#include <QDebug>
#include <QVariant>

Webcam::Webcam(QObject* parent) : QObject(parent) {
#ifdef ENV_RELEASE
    // --- Video Setup ---
    // --- Video Setup ---
    const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
    QCameraDevice selectedCamera = QMediaDevices::defaultVideoInput();

    for (const auto &cam : cameras) {
        qInfo() << "[Webcam] Found camera:" << cam.description();
        if (cam.description().contains("C210") || cam.description().contains("Logitech")) {
            selectedCamera = cam;
            break;
        }
    }

    m_camera = new QCamera(selectedCamera, this);

    connect(m_camera, &QCamera::errorOccurred, this, [](QCamera::Error error, const QString &errorString) {
        qWarning() << "[Webcam] Camera error occurred:" << error << errorString;
    });
    qInfo() << "[Webcam] Bound to:" << selectedCamera.description();
    m_session = new QMediaCaptureSession(this);
    m_videoSink = new QVideoSink(this);

    m_session->setCamera(m_camera);
    m_session->setVideoSink(m_videoSink);

    // Throttle frames to 10 FPS for Foxglove
    connect(&m_throttleTimer, &QTimer::timeout, this, [this]() {
        m_readyForNextFrame = true;
    });
    m_throttleTimer.start(100);
    connect(m_videoSink, &QVideoSink::videoFrameChanged, this, &Webcam::processFrame);

    // --- Audio Setup ---
    QAudioFormat format;
    format.setSampleRate(m_sampleRate);
    format.setChannelCount(m_channels);
    format.setSampleFormat(QAudioFormat::Int16);

    const QList<QAudioDevice> audioDevices = QMediaDevices::audioInputs();
    QAudioDevice selectedAudio = QMediaDevices::defaultAudioInput();

    // Loop through and find the C210 microphone
    for (const auto &mic : audioDevices) {
        qInfo() << "[Webcam] Found mic:" << mic.description();
        if (mic.description().contains("C210") || mic.description().contains("Logitech")) {
            selectedAudio = mic;
            break;
        }
    }

    qInfo() << "[Webcam] Bound audio to:" << selectedAudio.description();

    if (!selectedAudio.isFormatSupported(format)) {
        qWarning() << "[Webcam] Format not supported by this mic. Audio may sound distorted.";
    }
    m_audioSource = new QAudioSource(selectedAudio, format, this);
#else
    qInfo() << "[Webcam] Development build detected. Hardware AV capture disabled.";
#endif
}

void Webcam::start() {
#ifdef ENV_RELEASE
    m_camera->start();
    qInfo() << "[Webcam] Started capture on default camera device";
    startAudio();
#else
    qInfo() << "[Webcam] start() ignored in development build.";
#endif
}

void Webcam::setQmlVideoOutput(QObject* qmlOutput) {
#ifdef ENV_RELEASE
    if (!qmlOutput) return;

    m_session->setVideoOutput(qmlOutput);

    QVideoSink* qmlSink = qvariant_cast<QVideoSink*>(qmlOutput->property("videoSink"));
    if (qmlSink) {
        disconnect(m_videoSink, &QVideoSink::videoFrameChanged, this, &Webcam::processFrame);
        connect(qmlSink, &QVideoSink::videoFrameChanged, this, &Webcam::processFrame);
        qInfo() << "[Webcam] Routed video frames to QML UI";
    }
#else
    Q_UNUSED(qmlOutput);
    qInfo() << "[Webcam] setQmlVideoOutput() ignored in development build.";
#endif
}

#ifdef ENV_RELEASE
void Webcam::startAudio() {
    if (QMediaDevices::audioInputs().isEmpty()) {
        qWarning() << "[Webcam] No audio devices found yet. Retrying in 2 seconds...";
        QTimer::singleShot(2000, this, &Webcam::startAudio);
        return;
    }

    m_ioDevice = m_audioSource->start();
    if (m_ioDevice) {
        connect(m_ioDevice, &QIODevice::readyRead, this, &Webcam::processAudio);
        qInfo() << "[Webcam] Started capturing live audio";
    } else {
        qWarning() << "[Webcam] Failed to open audio hardware";
    }
}

void Webcam::processFrame(const QVideoFrame& frame) {
    if (!m_readyForNextFrame) return;

    if (m_camera && m_camera->error() != QCamera::NoError) {
        return;
    }

    if (!frame.isValid()) return;

    m_readyForNextFrame = false;

    QVideoFrame cloneFrame = frame;
    if (!cloneFrame.isValid()) return;

    QImage image = cloneFrame.toImage();
    if (image.isNull()) return;

    QImage scaled = image.scaled(640, 480, Qt::KeepAspectRatio, Qt::FastTransformation);
    QImage flipped = scaled.mirrored(true, true);

    emit frameReady("/camera/front", flipped.convertToFormat(QImage::Format_RGB32));
}

void Webcam::processAudio() {
    if (!m_ioDevice || !m_audioSource) return;

    if (m_audioSource->error() != QAudio::NoError) {
        qWarning() << "[Webcam] Audio device error detected. Stopping audio stream.";
        m_audioSource->stop();
        m_ioDevice = nullptr;
        return;
    }

    QByteArray data = m_ioDevice->readAll();
    if (data.isEmpty()) return;

    m_audioBuffer.append(data);

    int bytesPerSecond = m_sampleRate * m_channels * 2;
    int chunkSize = bytesPerSecond / 10;

    if (m_audioBuffer.size() >= chunkSize) {
        emit audioReady("/camera/audio_stream", m_audioBuffer, m_sampleRate, m_channels);
        m_audioBuffer.clear();
    }
}
#endif