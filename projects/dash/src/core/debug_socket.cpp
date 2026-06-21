#include "debug_socket.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>

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
    m_mockRpm += 10.5;
    if (m_mockRpm > 8000.0) m_mockRpm = 0.0;

    QJsonObject json;
    json["canId"] = 0x100;
    json["RPM"] = m_mockRpm;
    json["Speed"] = m_mockRpm * 0.015;

    emit uiDataUpdated("RPM", m_mockRpm);
    emit uiDataUpdated("Speed", json["Speed"].toDouble());

    QByteArray payload = QJsonDocument(json).toJson(QJsonDocument::Compact);
    emit foxglovePayloadReady(payload);
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