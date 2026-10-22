#include "cansocket.h"
#include <QCanBus>
#include <QVariant>
#include <QDebug>

CanSocket::CanSocket(QObject *parent)
    : QObject(parent)
{
}

bool CanSocket::start(const QString &plugin, const QString &interfaceName)
{
    QString selectedPlugin = plugin;
    if (selectedPlugin.isEmpty()) {
#ifdef Q_OS_LINUX
        selectedPlugin = QStringLiteral("socketcan");
#else
        selectedPlugin = QStringLiteral("virtualcan");
#endif
    }

    QString errorMsg;
    m_device = QCanBus::instance()->createDevice(selectedPlugin, interfaceName, &errorMsg);
    if (!m_device) {
        qWarning() << "createDevice failed:" << errorMsg;
        return false;
    }

    m_device->setParent(this);

    m_device->setConfigurationParameter(QCanBusDevice::CanFdKey, QVariant(true));

    connect(m_device, &QCanBusDevice::framesReceived,
            this, &CanSocket::onDeviceFramesReceived);

    connect(m_device, &QCanBusDevice::stateChanged,
            this, [selectedPlugin, interfaceName](QCanBusDevice::CanBusDeviceState state) {
        if (state == QCanBusDevice::ConnectedState) {
            qInfo() << "Connected to CAN backend:" << selectedPlugin << "on" << interfaceName;
        }
    });

    if (!m_device->connectDevice()) {
        qWarning() << "connectDevice failed:" << m_device->errorString();
        delete m_device;
        m_device = nullptr;
        return false;
    }

    return true;
}

void CanSocket::onDeviceFramesReceived()
{
    const QList<QCanBusFrame> frames = m_device->readAllFrames();
    if (!frames.isEmpty())
        emit framesReceived(frames);
}