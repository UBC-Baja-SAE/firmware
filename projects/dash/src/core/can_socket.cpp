#include "can_socket.h"
#include <QCanBus>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>
#include <QTime>
#include <cstring> // Required for memcpy float casting

namespace {
    const QHash<QString, QString> kSignalTopicMap = {
        {"speedometer",      "/speed"},
        {"tachometer",         "/rpm"},
        {"travel",  "/travel"},
        {"accelX", "/imu/accel"}, {"accelY", "/imu/accel"}, {"accelZ", "/imu/accel"},
        {"gyroX",  "/imu/gyro"},  {"gyroY",  "/imu/gyro"},  {"gyroZ",  "/imu/gyro"},
    };
}

namespace {
    // CANopen TPDO mappings
    // Base IDs: TPDO1=384(0x180), TPDO2=640(0x280), TPDO3=896(0x380), TPDO4=1152(0x480)
    // Heartbeats: 1792 (0x700) + NodeID
    const QHash<quint32, QString> kEcuPrefixMap = {
        // Node 1: RR
        {385, "rr_ecu"}, {641, "rr_ecu"}, {897, "rr_ecu"}, {1153, "rr_ecu"}, {1793, "rr_ecu"},

        // Node 2: RL
        {386, "rl_ecu"}, {642, "rl_ecu"}, {898, "rl_ecu"}, {1154, "rl_ecu"}, {1794, "rl_ecu"},

        // Node 3: FR
        {387, "fr_ecu"}, {643, "fr_ecu"}, {899, "fr_ecu"}, {1155, "fr_ecu"}, {1795, "fr_ecu"},

        // Node 4: FL
        {388, "fl_ecu"}, {644, "fl_ecu"}, {900, "fl_ecu"}, {1156, "fl_ecu"}, {1796, "fl_ecu"},

        // Rear ECU (assuming it still uses ID 1280 / 0x500)
        {1280, "rear_ecu"},
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

#ifdef Q_OS_LINUX
    const QString pluginName = QStringLiteral("socketcan");
    qInfo() << "[Can] Using socketcan plugin.";
#else
    const QString pluginName = QStringLiteral("virtualcan");
    qInfo() << "[VCan] Using virtualcan plugin.";
#endif

    QString errorString;
    m_device = QCanBus::instance()->createDevice(pluginName, m_interfaceName, &errorString);
    if (!m_device) {
        qWarning() << "Error creating CAN device:" << errorString;
        return;
    }

    m_device->setConfigurationParameter(QCanBusDevice::CanFdKey, true);
    connect(m_device, &QCanBusDevice::framesReceived, this, &CanWorker::processFrames);

    if (m_device->connectDevice()) {
        qInfo() << "Connected to" << m_interfaceName << "with CAN FD enabled.";

        // AUTO-BOOT NMT COMMAND: Broadcast 'Set Operational' to all nodes (0x00)
        sendNmtOperational(0x00);

    } else {
        qWarning() << "Failed to connect to CAN device.";
    }
}

void CanWorker::sendNmtOperational(quint8 nodeId) {
    if (!m_device || m