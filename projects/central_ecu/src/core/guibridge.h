#pragma once
#include <QObject>
#include <QQmlPropertyMap>
#include <QTimer>
#include <QVariantMap>
#include <QString>
#include <QtMath>
#include <QCanUniqueIdDescription>

class GuiBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQmlPropertyMap* values READ values CONSTANT)
    Q_PROPERTY(bool    recording READ recording NOTIFY recordingChanged)
    Q_PROPERTY(double  cpuTempC  READ cpuTempC  NOTIFY cpuTempChanged)
    Q_PROPERTY(QString ipAddress READ ipAddress NOTIFY ipAddressChanged)
public:
    explicit GuiBridge(QObject *parent = nullptr);
    QQmlPropertyMap *values() { return &m_values; }

    bool    recording() const { return m_recording; }
    double  cpuTempC()  const { return m_cpuTempC; }
    QString ipAddress() const { return m_ip; }

public slots:
    void onDecoded(QtCanBus::UniqueId id, const QVariantMap &signalValues, quint64 timestampNs);
    void setRecording(bool on);

    signals:
        void recordingChanged();
    void cpuTempChanged();
    void ipAddressChanged();

private:
    void flush();
    void pollSystem();

    QQmlPropertyMap m_values;
    QVariantMap     m_pending;
    QTimer          m_timer;

    bool    m_recording = false;
    double  m_cpuTempC  = qQNaN();
    QString m_ip        = QStringLiteral("--");
    QTimer  m_sysTimer;
};