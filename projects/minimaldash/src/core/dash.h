#ifndef DASH_H
#define DASH_H

#include <QObject>
#include <QJsonObject>
#include <QJsonValue>

class Dash : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int speed READ speed NOTIFY speedChanged)
    Q_PROPERTY(int rpm READ rpm NOTIFY rpmChanged)

public:
    explicit Dash(QObject *parent = nullptr) : QObject(parent), m_speed(0), m_rpm(0) {}

    int speed() const { return m_speed; }
    int rpm() const { return m_rpm; }

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
    }

    signals:
        void speedChanged();
    void rpmChanged();

private:
    int m_speed;
    int m_rpm;
};

#endif // DASH_H