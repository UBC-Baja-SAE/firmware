#ifndef DASH_H
#define DASH_H

#include <QObject>
#include <QJsonObject>
#include <QJsonValue>
#include <QCanBusFrame>
#include <QDebug>

class Dash : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int speed READ speed NOTIFY speedChanged)
    Q_PROPERTY(int rpm READ rpm NOTIFY rpmChanged)
    Q_PROPERTY(double steeringAngle READ steeringAngle NOTIFY steeringAngleChanged)
    Q_PROPERTY(bool isLogging READ isLogging NOTIFY isLoggingChanged)

public:
    explicit Dash(QObject *parent = nullptr)
        : QObject(parent), m_speed(0), m_rpm(0), m_steering(0.0), m_isLogging(false) {}

    int speed() const { return m_speed; }
    int rpm() const { return m_rpm; }
    double steeringAngle() const { return m_steering; }
    bool isLogging() const { return m_isLogging; }

public slots:
    void onFrameParsed(const QString &topicName, const QJsonObject &payload) {
        QString topic = topicName.toLower();

        if (topic == "speedometer") {
            if (payload.contains("speed")) {
                m_speed = payload["speed"].toDouble();
                emit speedChanged();
            }
        }
        else if (topic == "tachometer") {
            if (payload.contains("rpm")) {
                m_rpm = payload["rpm"].toDouble();
                emit rpmChanged();
            }
        }
        else if (topic == "front_steering") {
            if (payload.contains("steering_angle")) {
                m_steering = payload["steering_angle"].toDouble();
                emit steeringAngleChanged();
            }
        }

        if (topicName == "front_buttons") {
            int btn_id = payload["button_id"].toInt();
            int state = payload["button_state"].toInt();
            int seq = payload["seq"].toInt();

            // Toggle Switch is Button ID 3
            if (btn_id == 3) {
                bool newLoggingState = (state == 1);
                if (m_isLogging != newLoggingState) {
                    m_isLogging = newLoggingState;
                    qDebug() << "[DASH] Logging state changed to:" << m_isLogging;
                    emit isLoggingChanged();
                    emit loggingCommanded(m_isLogging); // Tell main.cpp to start/stop Foxglove
                }
            }

            // Always ACK the button back to the STM32
            QByteArray ackPayload;
            ackPayload.resize(3);
            ackPayload[0] = static_cast<char>(btn_id);
            ackPayload[1] = 0x01;
            ackPayload[2] = static_cast<char>(seq);

            QCanBusFrame ackFrame(1280, ackPayload);
            emit outboundFrameReady(ackFrame);
        }

        emit frameForwardedToQml(topicName, payload);
    }

signals:
    void speedChanged();
    void rpmChanged();
    void steeringAngleChanged();
    void isLoggingChanged();

    void loggingCommanded(bool enable); // Emitted when toggle switch flips
    void frameForwardedToQml(const QString &topicName, const QJsonObject &payload);
    void outboundFrameReady(const QCanBusFrame &frame);

private:
    int m_speed;
    int m_rpm;
    double m_steering;
    bool m_isLogging;
};

#endif // DASH_H