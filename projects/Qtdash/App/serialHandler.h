#pragma once
#include <QObject>
#include <QByteArray>
#include <QElapsedTimer>

class QSerialPort;

class SerialHandler : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool gpioHigh READ gpioHigh NOTIFY gpioHighChanged)

public:
    explicit SerialHandler(QObject* parent = nullptr);
    ~SerialHandler();

    bool open(const QString& port_name, int baud_rate);
    void close();
    bool isConnected() const;
    bool gpioHigh() const { return gpioHigh_; }

signals:
    void lineReceived(const QString& port, const QString& line);
    void connectedChanged(bool connected);
    void errorOccurred(const QString& error);
    void gpioHighChanged(bool high);
    void paddle1Pressed();
    void paddle2Pressed();
    void button1Pressed();
    void button2Pressed();
    void button3Pressed();
    void button4Pressed();

private slots:
    void onReadyRead();
    void onErrorOccurred();

private:
    static constexpr int kDebounceMsec    = 200;
    static constexpr int kT2DebounceMsec  = 1000;

    QSerialPort*  port_    = nullptr;
    QString       portName_;
    QByteArray    buffer_;

    QElapsedTimer paddle1Timer_;
    QElapsedTimer paddle2Timer_;
    QElapsedTimer button1Timer_;
    QElapsedTimer button2Timer_;
    QElapsedTimer button3Timer_;
    QElapsedTimer button4Timer_;
    QElapsedTimer t2Timer_;

    static constexpr int kGpioSysfsNum = 588;
    bool gpioHigh_ = false;
    void initGpio();
    void cleanupGpio();
    void toggleGpio();
};