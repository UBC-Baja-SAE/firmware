#include "cansocket.h"
#include <QCanBus>
#include <QDebug>
#include <QVariant>

CanSocket::CanSocket(QObject *parent)
    : QObject(parent)
{
}

bool CanSocket::connectDevice()
{
#ifdef LINUX
    const QString deviceName = QStringLiteral("socketcan");
#else
    const QString deviceName = QStringLiteral("virtualcan");
#endif
    const QString deviceNum = QStringLiteral("can0");

    QString errorMsg;
    m_device = QCanBus::instance()->createDevice(deviceName, deviceNum, &errorMsg);
    if (!m_device) {
        qWarning() << "createDevice failed:" << errorMsg;
        return false;
    }
    
    connect(m_device, &QCanBusDevice::framesReceived, this, &CanSocket::onFramesReceived);

    if (!m_device->connectDevice()) {
        qWarning() << "connectDevice failed:" << m_device->errorString();
        delete m_device;
        m_device = nullptr;
        return false;
    }

    return true;
}

void CanSocket::onFramesReceived()
{
    while (m_device->framesAvailable() > 0) {
        const QCanBusFrame frame = m_device->readFrame();
        emit rawFrameReceived(frame);
    }
}