#ifndef DASHBOARD_QT_UART_HANDLER_H
#define DASHBOARD_QT_UART_HANDLER_H

#include <QObject>
#include <QSerialPort>

class UARTHandler : public QObject {
    Q_OBJECT
public:
    explicit UARTHandler(QObject *parent = nullptr);
    bool connectPort(const QString &portName);

    signals:
        void modeChanged(QString newMode); // Signal to tell the UI to update

private slots:
    void readSerial();

private:
    QSerialPort *serial;
};

#endif //DASHBOARD_QT_UART_HANDLER_H