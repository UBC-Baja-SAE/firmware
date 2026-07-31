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
        : QObject(parent), m_speed(0), m_rpm(0), m_rawSteering(0.0), m_steeringOffset(0.0) {}

    int speed() const { return m_speed; }
    int rpm() const { return m_rpm; }
    double steeringAngle() const { return m_rawSteering - m_steeringOffset; }

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
                m_rawSteering = payload["steering_angle"].toDouble();
                emit steeringAngleChanged();
            }
        }

        if (topicName == "front_buttons") {
            int btn_id = payload["button_id"].toInt();
            int state = payload["button_state"].toInt();
            int seq = payload["seq"].toInt();

            // Steering Calibration on Button 2 Press
            if (btn_id == 2 && state == 1) {
                m_steeringOffset = m_rawSteering;
                qDebug() << "[DASH] Steering calibrated! New offset:" << m_steeringOffset;
                emit steeringAngleChanged();
            }

            // Construct the 3-byte ACK payload
            QByteArray ackPayload;
            ackPayload.resize(3);
            ackPayload[0] = static_cast<char>(btn_id);
            ackPayload[1] = 0x01; // 1 = ACK OK
            ackPayload[2] = static_cast<char>(seq);

            // Create the frame (ID 1280 = 0x500) and emit it
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
    double m_rawSteering;
    double m_steeringOffset;
};

#endif // DASH_H