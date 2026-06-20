#include "can_socket.h"

CanSocket::CanSocket(QObject *parent) : QObject(parent) {}

CanSocket::~CanSocket() {}

bool CanSocket::connectToDevice(const QString &interfaceName) {
    qDebug() << "Desktop build: CAN interface mocked. Requested connection to" << interfaceName;
    // TODO: Add support for can interfaces on mac/windows for debugging
    return true;
}

void CanSocket::processReceivedFrames() {}

void CanSocket::handleDeviceError(QCanBusDevice::CanBusError error) {}