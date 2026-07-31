#pragma once
#include <QObject>
#include <QCanBusDevice>

class CanSocket : public QObject
{
    Q_OBJECT
public:
    explicit CanSocket(QObject *parent = nullptr);

public slots:
    bool connectDevice();
    void sendFrame(const QCanBusFrame &frame);

    signals:
    void rawFrameReceived(const QCanBusFrame &frame);

private slots:
    void onFramesReceived();



private:
    QCanBusDevice *m_device = nullptr;
};
