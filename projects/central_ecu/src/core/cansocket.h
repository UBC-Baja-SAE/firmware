#pragma once

#include <QObject>
#include <QCanBusDevice>
#include <QCanBusFrame>
#include <QList>
#include <QString>

class CanSocket : public QObject
{
    Q_OBJECT

public:
    explicit CanSocket(QObject *parent = nullptr);
    bool start(const QString &plugin = {}, const QString &interfaceName = QStringLiteral("can0"));

    signals:
        void framesReceived(const QList<QCanBusFrame> &frames);

private slots:
    void onDeviceFramesReceived();

private:
    QCanBusDevice *m_device = nullptr;
};