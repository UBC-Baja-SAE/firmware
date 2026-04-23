#ifndef SERIALHANDLER_H
#define SERIALHANDLER_H

#include <QObject>
#include <QSerialPort>

class SerialHandler : public QObject
{
    Q_OBJECT
public:
    explicit SerialHandler(QObject *parent = nullptr);

signals:
    // These signals will be caught by QML
    void paddle1Pressed();
    void paddle2Pressed();

private slots:
    void readData();

private:
    QSerialPort m_serial;
    QByteArray m_buffer;
};

#endif // SERIALHANDLER_H