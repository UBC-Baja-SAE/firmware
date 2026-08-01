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

public:
    explicit Dash(QObject *parent = nullptr)
        : QObject(parent), m_speed(0), m_rpm(0), m_steering(0.0) {}

    int speed() const { return m_speed; }
    int rpm() const { return m_rpm; }
    double steeringAngle() const { return m_steering; } // Pass through directly

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
                // Store the already-calibrated value directly
                m_steering = payload["steering_angle"].toDouble();
                emit steeringAngleChanged();
            }
        }

        // Keep the ACK logic so the STM32 knows we heard the button press
        if (topicName == "front_buttons") {
            int btn_id = payload["button_id"].toInt();
            int seq = payload["seq"].toInt();

            QByteArray ackPayload;
            ackPayload.resize(3);
            ackPayload[0] = static_cast<char>(btn_id);
            ackPayload[1] = 0x01; // 1 = ACK OK
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
    void frameForwardedToQml(const QString &topicName, const QJsonObject &payload);
    void outboundFrameReady(const QCanBusFrame &frame);

private:
    int m_speed;
    int m_rpm;
    double m_steering;
};

#endif // DASH_H