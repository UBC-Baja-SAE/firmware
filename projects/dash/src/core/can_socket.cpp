#include "can_socket.h"
#include <QtCore/qstringliteral.h>
#include <QVariant>

using namespace Qt::StringLiterals;

CanSocket::CanSocket(QObject *parent) : QObject(parent) {}

CanSocket::~CanSocket() {
    if (m_canDevice) {
        m_canDevice->disconnectDevice();
        delete m_canDevice;
    }
}

bool CanSocket::connectToDevice(const QString &interfaceName) {
    if (!QCanBus::instance()->plugins().contains(u"socketcan"_s)) {
        qCritical() << "SocketCAN plugin is not available in this Qt build.";
        return false;
    }

    QString errorString;
    m_canDevice = QCanBus::instance()->createDevice(u"socketcan"_s, interfaceName, &errorString);

    if (!m_canDevice) {
        qCritical() << "Error creating CAN device:" << errorString;
        return false;
    }

    // Explicitly enable CAN FD
    m_canDevice->setConfigurationParameter(QCanBusDevice::CanFdKey, QVariant(true));

    connect(m_canDevice, &QCanBusDevice::framesReceived, this, &CanSocket::processReceivedFrames);
    connect(m_canDevice, &QCanBusDevice::errorOccurred, this, &CanSocket::handleDeviceError);

    if (!m_canDevice->connectDevice()) {
        qCritical() << "CAN connection error:" << m_canDevice->errorString();
        delete m_canDevice;
        m_canDevice = nullptr;
        return false;
    }

    qDebug() << "Successfully connected to" << interfaceName << "with CAN FD enabled.";
    return true;
}

void CanSocket::processReceivedFrames() {
    if (!m_canDevice) return;

    while (m_canDevice->framesAvailable()) {
        QCanBusFrame frame = m_canDevice->readFrame();

        if (frame.frameType() == QCanBusFrame::DataFrame) {
            // TODO: process frames
            emit frameReceived(frame.frameId(), frame.payload());

        }
    }
}

void CanSocket::handleDeviceError(QCanBusDevice::CanBusError error) {
    qWarning() << "QCanBus Error:" << error << m_canDevice->errorString();
}