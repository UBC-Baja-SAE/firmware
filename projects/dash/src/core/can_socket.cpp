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
    if (!m_device || m_device->state() != QCanBusDevice::ConnectedState) {
        qWarning() << "Cannot send NMT: CAN device not connected.";
        return;
    }

    QCanBusFrame frame;
    frame.setFrameId(0x000); // NMT COB-ID is always 0x000
    frame.setExtendedFrameFormat(false); // Standard 11-bit ID

    QByteArray payload;
    payload.append(static_cast<char>(0x01)); // CS: 0x01 = Start / Set Operational
    payload.append(static_cast<char>(nodeId)); // Node ID (0x00 = broadcast to all)

    frame.setPayload(payload);

    if (!m_device->writeFrame(frame)) {
        qWarning() << "Failed to send NMT Operational frame:" << m_device->errorString();
    } else {
        qInfo() << "Sent NMT Operational command to Node:" << nodeId;
    }
}

void CanWorker::processFrames() {
    if (!m_device) return;

    while (m_device->framesAvailable()) {
        QCanBusFrame frame = m_device->readFrame();

        // --- CANopen Auto-Wakeup (Self-Healing) ---
        // Heartbeat COB-IDs are 0x700 + NodeID. If payload is 0x7F, it's Pre-Operational.
        if (frame.frameId() >= 0x701 && frame.frameId() <= 0x77F) {
            if (!frame.payload().isEmpty() && frame.payload().at(0) == 0x7F) {
                quint8 nodeId = frame.frameId() - 0x700;
                qInfo() << "[CANopen] Auto-waking Node" << nodeId << "from Pre-Operational state";
                sendNmtOperational(nodeId);
            }
        }
        // ------------------------------------------

        // --- Emit Raw Frame for the Sniffer UI ---
        // Format time as HH:mm:ss.zzz, ID as zero-padded Hex, and Payload as space-separated Hex
        QString timeStr = QTime::currentTime().toString("HH:mm:ss.zzz");
        QString idStr = QString::number(frame.frameId(), 16).toUpper().rightJustified(3, '0');
        QString dataStr = frame.payload().toHex(' ').toUpper();

        emit rawFrameReceived(timeStr, idStr, dataStr);
        // -----------------------------------------

        auto result = m_frameProcessor.parseFrame(frame);

        if (result.signalValues.isEmpty()) continue;

        const QString ecuPrefix = kEcuPrefixMap.value(frame.frameId(), QStringLiteral("unknown_ecu"));

        for (auto it = result.signalValues.constBegin(); it != result.signalValues.constEnd(); ++it) {
            QString key = it.key();
            QVariant val = it.value();

            // BIT-CAST WORKAROUND: Convert Qt's integer parse back into an IEEE float
            if (key.startsWith("accel") || key.startsWith("gyro")) {
                qint32 rawBits = val.toInt();
                float realFloat;
                memcpy(&realFloat, &rawBits, sizeof(float));
                val = static_cast<double>(realFloat); // Json prefers doubles
            }

            emit uiDataUpdated(key, val);

            const QString topic = (key.startsWith("accel") || key.startsWith("gyro"))
                ? "/" + ecuPrefix + "/imu/" + key
                : "/" + ecuPrefix + "/" + key;

            QJsonObject json;
            json[key] = QJsonValue::fromVariant(val);
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

    // Route the sniffer signal from the worker thread to the main thread
    connect(m_worker, &CanWorker::rawFrameReceived, this, &CanSocket::rawFrameReceived);

    connect(&m_workerThread, &QThread::started, m_worker, &CanWorker::start);
    connect(&m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);

    m_workerThread.start();
}