#pragma once

#include <QObject>
#include <QCanBus>
#include <QCanBusDevice>
#include <QCanBusFrame>
#include <QDebug>

class CanSocket : public QObject {
    Q_OBJECT

public:
    explicit CanSocket(QObject *parent = nullptr);
    ~CanSocket();

    Q_INVOKABLE bool connectToDevice(const QString &interfaceName = "can0");

    signals:
        // Signal to forward payloads
        void frameReceived(quint32 id, const QByteArray &data);

private slots:
    void processReceivedFrames();
    void handleDeviceError(QCanBusDevice::CanBusError error);

private:
    QCanBusDevice *m_canDevice = nullptr;
};