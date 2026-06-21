#pragma once

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QVariant>  // Fix: undefined type QVariant
#include <QString>   // Fix: undefined type QString

// No Foxglove includes here!

class DebugWorker : public QObject {
    Q_OBJECT
public:
    explicit DebugWorker(QObject* parent = nullptr);
    ~DebugWorker();

public slots:
    void start();
    void stop();

    signals:
        void uiDataUpdated(const QString& signalName, QVariant value);
    void foxglovePayloadReady(const QByteArray& payload); // Emits raw JSON

private slots:
    void generateMockData();

private:
    QTimer* m_timer = nullptr;
    double m_mockRpm = 0.0;
};

class DebugSocket : public QObject {
    Q_OBJECT
public:
    explicit DebugSocket(QObject* parent = nullptr);
    ~DebugSocket();

    Q_INVOKABLE void connectToDevice(const QString& interfaceName);

    signals:
        void uiDataUpdated(const QString& signalName, QVariant value);
    void foxglovePayloadReady(const QByteArray& payload); // Forwards out of thread

private:
    QThread m_workerThread;
    DebugWorker* m_worker = nullptr;
};