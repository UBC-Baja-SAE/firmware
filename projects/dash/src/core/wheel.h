#ifndef WHEEL_H
#define WHEEL_H

#include <QObject>
#include <QThread>
#include <QByteArray>
#include <QString>
#include <atomic>

// Reads a quadrature rotary encoder (steering angle) on the Pi 5 GPIO header
// via libgpiod v2, decodes position in a dedicated thread (blocking edge-event
// wait, no polling), and periodically emits a Foxglove JSON payload compatible
// with FoxgloveServer::broadcastCanFrame.
//
// NOTE: On the Pi 5 the 40-pin header is NOT /dev/gpiochip0 -- it's behind the
// RP1 south-bridge chip. This class auto-detects it by label ("pinctrl-rp1")
// instead of hardcoding a chip path/number.

class WheelWorker : public QObject {
    Q_OBJECT
public:
    WheelWorker(int pinA, int pinB, int countsPerRev, QObject* parent = nullptr);

public slots:
    void process();  // entry point once moved to the worker thread
    void stop();
    void zero();      // re-zero accumulated angle (e.g. at startup/centering)

    signals:
        void foxglovePayloadReady(const QString& topic, const QByteArray& payload);
    void finished();

private:
    int m_pinA;
    int m_pinB;
    int m_countsPerRev;                     // encoder PPR (lines/rev), not x4 counts
    std::atomic_bool m_running{true};
    std::atomic_bool m_zeroRequested{false};
    std::atomic<long long> m_position{0};   // raw quadrature counts (x4)

    static QString findRP1ChipPath();
    void emitAngle();
};

class Wheel : public QObject {
    Q_OBJECT
public:
    // pinA/pinB are BCM GPIO numbers (e.g. 3, 4). countsPerRev is the
    // encoder's pulses-per-revolution rating from its datasheet.
    explicit Wheel(int pinA = 3, int pinB = 4, int countsPerRev = 600, QObject* parent = nullptr);
    ~Wheel() override;

    void start();
    void stop();

public slots:
    void zero();

    signals:
        void foxglovePayloadReady(const QString& topic, const QByteArray& payload);

private:
    QThread m_thread;
    WheelWorker* m_worker;
};

#endif // WHEEL_H