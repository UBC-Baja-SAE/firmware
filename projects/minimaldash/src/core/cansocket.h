#pragma once

#include <QObject>
#include <QCanBusDevice>

class CanSocket : public QObject
{
    Q_OBJECT

public:
    explicit CanSocket(QObject *parent = nullptr);

    bool connectDevice();

private slots:
    void onFramesReceived();

private:
    QCanBusDevice *m_device = nullptr;
};