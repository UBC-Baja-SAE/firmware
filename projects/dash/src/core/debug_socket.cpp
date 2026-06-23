#include "debug_socket.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>
#include <QCanBus>
#include <QCanBusDevice>
#include <cstdlib>
#include <cmath>

DebugWorker::DebugWorker(QObject* parent) : QObject(parent) {}

DebugWorker::~DebugWorker() {
    stop();
}

void DebugWorker::start(const QString& channelName) {
    if (!QCanBus::instance()->plugins().contains(QStringLiteral("virtualcan"))) {
        qWarning() << "[VCan] virtualcan plugin not available in this Qt build - falling back to mock data";
        startMockGenerator();
        return;
    }

    QString errorString;
    m_canDevice = QCanBus::instance()->createDevice(
        QStringLiteral("virtualcan"), channelName, &errorString);

    if (!m_canDevice) {
        qWarning() << "[VCan] Failed to create virtualcan device:" << errorString
                    << "- falling back to mock data";
        startMockGenerator();
        return;
    }

    connect(m_canDevice, &QCanBusDevice::framesReceived, this, &DebugWorker::handleFrames);
    connect(m_canDevice, &QCanBusDevice::errorOccurred, this, [this](QCanBusDevice::CanBusError) {
        qWarning() << "[VCan] virtualcan error:" << m_canDevice->errorString();
    });

    if (!m_canDevice->connectDevice()) {
        qWarning() << "[VCan] virtualcan connectDevice() failed:" << m_canDevice->errorString()
                    << "- falling back to mock data";
        startMockGenerator();
        return;
    }

    qInfo() << "[VCan] Listening for real CAN frames on virtualcan channel" << channelName
             << "(maps to numeric channel 0')";
}

void DebugWorker::handleFrames() {
    while (m_canDevice && m_canDevice->framesAvailable()) {
        const QCanBusFrame frame = m_canDevice->readFrame();
        // TODO: route this through your existing QCanFrameProcessor / mochi.dbc
        // decode path instead of re-emitting raw, e.g.:
        //   m_frameProcessor->processFrame(frame);
        qDebug() << "[VCan] RX frame  id:" << Qt::hex << frame.frameId()
                 << "payload:" << frame.payload().toHex();

        emit uiDataUpdated("rawFrameId", frame.frameId());
    }
}

void DebugWorker::startMockGenerator() {
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &DebugWorker::generateMockData);
    m_timer->start(50); // 20Hz mock data
}

void DebugWorker::generateMockData() {
    // Independent targets
    static double targetRpm = 0.0;
    static double targetSpeed = 0.0;

    // Independent speed tracker (m_mockRpm is already tracked in your class)
    static double currentSpeed = 0.0;

    // --- 1. TACHOMETER LOGIC (0-4000 RPM) ---
    m_mockRpm += (targetRpm - m_mockRpm) * 0.05; // Snappy engine response

    if (std::abs(targetRpm - m_mockRpm) < 50.0) {
        targetRpm = static_cast<double>(std::rand() % 4001);
    }

    // --- 2. SPEEDOMETER LOGIC (0-60 km/h) ---
    currentSpeed += (targetSpeed - currentSpeed) * 0.03; // Slower, heavier vehicle inertia

    if (std::abs(targetSpeed - currentSpeed) < 1.0) {
        targetSpeed = static_cast<double>(std::rand() % 61);
    }

    // --- 3. QML UI EMISSIONS ---
    emit uiDataUpdated("tachometer", m_mockRpm);
    emit uiDataUpdated("speedometer", currentSpeed);

    // --- 4. FOXGLOVE JSON PAYLOAD ---
    QJsonObject ecuJson;
    ecuJson["tachometer"] = m_mockRpm;
    ecuJson["speedometer"] = currentSpeed;
    ecuJson["canId"] = 0x500; // 0x500 maps to "rear_ecu"

    QByteArray payload = QJsonDocument(ecuJson).toJson(QJsonDocument::Compact);
    emit foxglovePayloadReady("/rear_ecu", payload);
}

void DebugWorker::stop() {
    if (m_timer) {
        m_timer->stop();
    }
    if (m_canDevice) {
        m_canDevice->disconnectDevice();
        m_canDevice->deleteLater();
        m_canDevice = nullptr;
    }
}

DebugSocket::DebugSocket(QObject* parent) : QObject(parent) {}

DebugSocket::~DebugSocket() {
    if (m_workerThread.isRunning()) {
        m_workerThread.quit();
        m_workerThread.wait();
    }
}

void DebugSocket::connectToDevice(const QString& interfaceName) {
    const QString channel = interfaceName.isEmpty() ? QStringLiteral("can0") : interfaceName;

    m_worker = new DebugWorker();
    m_worker->moveToThread(&m_workerThread);

    connect(m_worker, &DebugWorker::uiDataUpdated, this, &DebugSocket::uiDataUpdated);
    connect(m_worker, &DebugWorker::foxglovePayloadReady, this, &DebugSocket::foxglovePayloadReady);

    connect(&m_workerThread, &QThread::started, m_worker, [this, channel]() {
        m_worker->start(channel);
    });
    connect(&m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);

    m_workerThread.start();
}