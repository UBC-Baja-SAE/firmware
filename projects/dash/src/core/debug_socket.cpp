#include "debug_socket.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>
#include <cstdlib>
#include <cmath>

DebugWorker::DebugWorker(QObject* parent) : QObject(parent) {}

DebugWorker::~DebugWorker() {
    stop();
}

void DebugWorker::start() {
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
}

DebugSocket::DebugSocket(QObject* parent) : QObject(parent) {}

DebugSocket::~DebugSocket() {
    if (m_workerThread.isRunning()) {
        m_workerThread.quit();
        m_workerThread.wait();
    }
}

void DebugSocket::connectToDevice(const QString& interfaceName) {
    Q_UNUSED(interfaceName);

    m_worker = new DebugWorker();
    m_worker->moveToThread(&m_workerThread);

    connect(m_worker, &DebugWorker::uiDataUpdated, this, &DebugSocket::uiDataUpdated);
    connect(m_worker, &DebugWorker::foxglovePayloadReady, this, &DebugSocket::foxglovePayloadReady);
    
    connect(&m_workerThread, &QThread::started, m_worker, &DebugWorker::start);
    connect(&m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);

    m_workerThread.start();
}