#include "can_socket.h"
#include <QCanBus>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>


//TODO: update topics based on dbc
namespace {
    const QHash<QString, QString> kSignalTopicMap = {
        {"speedometer",      "/speed"},
        {"tachometer",         "/rpm"},
        {"travel",  "/travel"},
        {"accelX", "/imu/accel"}, {"accelY", "/imu/accel"}, {"accelZ", "/imu/accel"},
        {"gyroX",  "/imu/gyro"},  {"gyroY",  "/imu/gyro"},  {"=gyroZ",  "/imu/gyro"},
    };
}

namespace {
    const QHash<quint32, QString> kEcuPrefixMap = {
        {0x100, "rr_ecu"},  {0x101, "rr_ecu"},
        {0x200, "rl_ecu"},  {0x201, "rl_ecu"},
        {0x300, "fr_ecu"},  {0x301, "fr_ecu"},
        {0x400, "fl_ecu"},  {0x401, "fl_ecu"},
        {0x500, "rear_ecu"},
    };
}

CanWorker::CanWorker(const QString& interfaceName, QObject* parent)
    : QObject(parent), m_interfaceName(interfaceName) {}

CanWorker::~CanWorker() {
    stop();
}

void CanWorker::start() {
    if (m_dbcParser.parse(":/mochi.dbc")) {
        m_frameProcessor.setMessageDescriptions(m_dbcParser.messageDescriptions());
        m_frameProcessor.setUniqueIdDescription(QCanDbcFileParser::uniqueIdDescription());
        qInfo() << "DBC parsed OK," << m_dbcParser.messageDescriptions().size() << "message(s) loaded";
    } else {
        qWarning() << "Failed to parse DBC:" << m_dbcParser.errorString();
    }

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

        const QString ecuPrefix = kEcuPrefixMap.value(frame.frameId(), QStringLiteral("unknown_ecu"));

        for (auto it = result.signalValues.constBegin(); it != result.signalValues.constEnd(); ++it) {
            emit uiDataUpdated(it.key(), it.value());

            const QString topic = (it.key().startsWith("accel") || it.key().startsWith("gyro"))
                ? "/" + ecuPrefix + "/imu." + it.key()
                : "/" + ecuPrefix + "." + it.key();

            QJsonObject json;
            json[it.key()] = QJsonValue::fromVariant(it.value());
            json["canId"] = static_cast<qint64>(result.uniqueId);

            QByteArray payload = QJsonDocument(json).toJson(QJsonDocument::Compact);
            emit foxglovePayloadReady(topic, payload);
        }
    }
}

void CanWorker::stop() {
    if (m_device) {
        m_device->disconnectDevice();
        delete m_device;
        m_device = nullptr;
    }
}

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