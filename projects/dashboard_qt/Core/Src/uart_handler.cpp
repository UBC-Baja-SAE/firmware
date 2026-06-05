#include "../Inc/uart_handler.h"
#include <QVBoxLayout>


UARTHandler::UARTHandler(QObject *parent) : QObject(parent) {
    serial = new QSerialPort(this);

    serial->setBaudRate(QSerialPort::Baud115200);
    serial->setDataBits(QSerialPort::Data8);
    serial->setParity(QSerialPort::NoParity);
    serial->setStopBits(QSerialPort::OneStop);
    serial->setFlowControl(QSerialPort::NoFlowControl);

    connect(serial, &QSerialPort::readyRead, this, &UARTHandler::readSerial);
}

bool UARTHandler::connectPort(const QString &portName) {
    serial->setPortName(portName);


    return serial->open(QIODevice::ReadWrite);
}

void UARTHandler::readSerial() {
    while (serial->canReadLine()) {
        QString data = QString::fromUtf8(serial->readLine()).trimmed();

        if (data == "B1") emit modeChanged("Jump");
        else if (data == "B2") emit modeChanged("Bump");
        else if (data == "B3") emit modeChanged("Speed");
        else if (data == "B4") emit modeChanged("Corner");
    }
}