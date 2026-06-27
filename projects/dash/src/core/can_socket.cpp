#include "can_socket.h"
#include <QCanBus>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>
#include <QTime> // NEW: Required for sniffer timestamps


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

        // --- NEW: Emit Raw Frame for the Sniffer UI ---
        // Format time as HH:mm:ss.zzz, ID as zero-padded Hex, and Payload as space-separated Hex
        QString timeStr = QTime::currentTime().toString("HH:mm:ss.zzz");
        QString idStr = QString::number(frame.frameId(), 16).toUpper().rightJustified(3, '0');
        QString dataStr = frame.payload().toHex(' ').toUpper();

        emit rawFrameReceived(timeStr, idStr, dataStr);
        // ----------------------------------------------

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

    // NEW: Route the sniffer signal from the worker thread to the main thread
    connect(m_worker, &CanWorker::rawFrameReceived, this, &CanSocket::rawFrameReceived);

    connect(&m_workerThread, &QThread::started, m_worker, &CanWorker::start);
    connect(&m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);

    m_workerThread.start();
}