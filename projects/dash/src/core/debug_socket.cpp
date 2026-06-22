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
    static double targetRpm = 0.0;

    m_mockRpm += (targetRpm - m_mockRpm) * 0.05;

    if (std::abs(targetRpm - m_mockRpm) < 50.0) {
        targetRpm = static_cast<double>(std::rand() % 4001);
    }

    double mockSpeed = m_mockRpm * 0.015;

    emit uiDataUpdated("tachometer", m_mockRpm);

    QJsonObject tachJson;
    tachJson["tachometer"] = m_mockRpm;
    tachJson["canId"] = 0x500; // 0x500 maps to "rear_ecu"

    QByteArray tachPayload = QJsonDocument(tachJson).toJson(QJsonDocument::Compact);
    emit foxglovePayloadReady("/rear_ecu.tachometer", tachPayload);

    emit uiDataUpdated("speedometer", mockSpeed);

    QJsonObject speedJson;
    speedJson["speedometer"] = mockSpeed;
    speedJson["canId"] = 0x500; // 0x500 maps to "rear_ecu"

    QByteArray speedPayload = QJsonDocument(speedJson).toJson(QJsonDocument::Compact);
    emit foxglovePayloadReady("/rear_ecu.speedometer", speedPayload);
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