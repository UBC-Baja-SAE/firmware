#pragma once

#include <QObject>
#include <QThread>
#include <QVariant>
#include <QString>
#include <QHash>
#include <QCanBusDevice>
#include <QCanDbcFileParser>
#include <QCanFrameProcessor>
#include <QtSerialBus/QCanMessageDescription>
#include <QtSerialBus/QCanUniqueIdDescription>

class CanWorker : public QObject {
    Q_OBJECT
public:
    explicit CanWorker(const QString& interfaceName, QObject* parent = nullptr);
    ~CanWorker();

public slots:
    void start();
    void stop();
    void sendNmtOperational(quint8 nodeId = 0x00);

    signals:
        void uiDataUpdated(const QString& signalName, QVariant value);
    void foxglovePayloadReady(const QString& topic, const QByteArray& payload);

    // NEW: Signal to send raw sniffer data
    void rawFrameReceived(const QString& time, const QString& canId, const QString& data);

private slots:
    void processFrames();

private:
    QString m_interfaceName;
    QCanBusDevice* m_device = nullptr;
    QCanDbcFileParser m_dbcParser;
    QCanFrameProcessor m_frameProcessor;
};

class CanSocket : public QObject {
    Q_OBJECT
public:
    explicit CanSocket(QObject* parent = nullptr);
    ~CanSocket();

    Q_INVOKABLE void connectToDevice(const QString& interfaceName);

    signals:
        void uiDataUpdated(const QString& signalName, QVariant value);
    void foxglovePayloadReady(const QString& topic, const QByteArray& payload);

    // NEW: Signal exposed to QML
    void rawFrameReceived(const QString& time, const QString& canId, const QString& data);

private:
    QThread m_workerThread;
    CanWorker* m_worker = nullptr;
};