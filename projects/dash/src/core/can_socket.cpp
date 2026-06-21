#include "can_socket.h"
#include <QCanBus>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>

// --- CanWorker ---

CanWorker::CanWorker(const QString& interfaceName, QObject* parent)
    : QObject(parent), m_interfaceName(interfaceName) {}

CanWorker::~CanWorker() {
    stop();
}

void CanWorker::start() {
    // 1. Load DBC File
    if (m_dbcParser.parse("network.dbc")) {
        m_frameProcessor.setMessageDescriptions(m_dbcParser.messageDescriptions());
        m_frameProcessor.setUniqueIdDescription(QCanDbcFileParser::uniqueIdDescription());
    } else {
        qWarning() << "Failed to parse DBC:" << m_dbcParser.errorString();
    }

    // 2. Connect to SocketCAN
    QString errorString;
    m_device = QCanBus::instance()->createDevice("socketcan", m_interfaceName, &errorString);
    if (!m_device) {
        qWarning() << "Error creating CAN device:" << errorString;
        return;
    }

    m_device->setConfigurationParameter(QCanBusDevice::CanFdKey, true);
    connect(m_device, &QCanBusDevice::framesReceived, this, &CanWorker::processFrames);

    if (m_device->connectDevice()) {
        qInfo() << "Connected to" << m_interfaceName << "with CAN FD enabled.";
    } else {
        qWarning() << "Failed to connect to CAN device.";
    }
}

void CanWorker::processFrames() {
    if (!m_device) return;

    while (m_device->framesAvailable()) {
        QCanBusFrame frame = m_device->readFrame();
        auto result = m_frameProcessor.parseFrame(frame);

        if (result.signalValues.isEmpty()) continue;

        QJsonObject json;
        json["canId"] = static_cast<qint64>(result.uniqueId);

        for (auto it = result.signalValues.constBegin(); it != result.signalValues.constEnd(); ++it) {
            json[it.key()] = QJsonValue::fromVariant(it.value());
            emit uiDataUpdated(it.key(), it.value());
        }

        // Serialize and emit cleanly to the ether
        QByteArray payload = QJsonDocument(json).toJson(QJsonDocument::Compact);
        emit foxglovePayloadReady(payload);
    }
}

void CanWorker::stop() {
    if (m_device) {
        m_device->disconnectDevice();
        delete m_device;
        m_device = nullptr;
    }
}

// --- CanSocket ---

CanSocket::CanSocket(QObject* parent) : QObject(parent) {}

CanSocket::~CanSocket() {
    if (m_workerThread.isRunning()) {
        m_workerThread.quit();
        m_workerThread.wait();
    }
}

void CanSocket::connectToDevice(const QString& interfaceName) {
    m_worker = new CanWorker(interfaceName);
    m_worker->moveToThread(&m_workerThread);

    connect(m_worker, &CanWorker::uiDataUpdated, this, &CanSocket::uiDataUpdated);
    connect(m_worker, &CanWorker::foxglovePayloadReady, this, &CanSocket::foxglovePayloadReady);

    connect(&m_workerThread, &QThread::started, m_worker, &CanWorker::start);
    connect(&m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);

    m_workerThread.start();
}