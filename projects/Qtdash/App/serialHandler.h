#pragma once

#include <QObject>
#include <QString>

class QSerialPort;

// ─── SerialHandler ────────────────────────────────────────────────────────────
//
// Wraps a QSerialPort.  Every complete line received is:
//   1. emitted as lineReceived() so the QML UI can display it
//   2. forwarded to McapLogger::instance().logUartMessage()
//
// Typical usage (in main.cpp or wherever you set up serial):
//
//   auto* serial = new SerialHandler(this);
//   serial->open("/dev/ttyS0", 115200);
//

class SerialHandler : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)

public:
    explicit SerialHandler(QObject* parent = nullptr);
    ~SerialHandler() override;

    Q_INVOKABLE bool open(const QString& port_name, int baud_rate = 115200);
    Q_INVOKABLE void close();
    bool isConnected() const;

signals:
    void lineReceived(const QString& port_name, const QString& line);
    void connectedChanged(bool connected);
    void errorOccurred(const QString& message);

private slots:
    void onReadyRead();
    void onErrorOccurred();

private:
    QSerialPort* port_  {nullptr};
    QString      portName_;
    QByteArray   buffer_;
};