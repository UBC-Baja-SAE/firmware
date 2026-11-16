#include "guibridge.h"

#include <QFile>
#include <QNetworkInterface>
#include <QNetworkAddressEntry>
#include <QtMath>

GuiBridge::GuiBridge(QObject *parent)
    : QObject(parent), m_values(this)
{
    m_timer.setInterval(16);   // 60 Hz
    connect(&m_timer, &QTimer::timeout, this, &GuiBridge::flush);
    m_timer.start();

    m_sysTimer.setInterval(1000);   // 1 Hz: CPU temp + IP
    connect(&m_sysTimer, &QTimer::timeout, this, &GuiBridge::pollSystem);
    m_sysTimer.start();
    pollSystem();
}

void GuiBridge::onDecoded(QtCanBus::UniqueId, const QVariantMap &signalValues, quint64)
{
    for (auto it = signalValues.cbegin(); it != signalValues.cend(); ++it)
        m_pending.insert(it.key(), it.value());
}

void GuiBridge::flush()
{
    if (m_pending.isEmpty())
        return;
    for (auto it = m_pending.cbegin(); it != m_pending.cend(); ++it)
        m_values.insert(it.key(), it.value());
    m_pending.clear();
}

void GuiBridge::setRecording(bool on)
{
    if (on == m_recording)
        return;
    m_recording = on;
    emit recordingChanged();
}

void GuiBridge::pollSystem()
{
    double t = qQNaN();
    QFile f(QStringLiteral("/sys/class/thermal/thermal_zone0/temp"));
    if (f.open(QIODevice::ReadOnly))
        t = f.readAll().trimmed().toInt() / 1000.0;

    const bool bothNaN = qIsNaN(t) && qIsNaN(m_cpuTempC);
    if (!bothNaN && t != m_cpuTempC) {
        m_cpuTempC = t;
        emit cpuTempChanged();
    }

    QString ip = QStringLiteral("--");
    const auto ifaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &iface : ifaces) {
        const auto fl = iface.flags();
        if (!(fl & QNetworkInterface::IsUp) || !(fl & QNetworkInterface::IsRunning)
            || (fl & QNetworkInterface::IsLoopBack))
            continue;
        const auto entries = iface.addressEntries();
        for (const QNetworkAddressEntry &e : entries) {
            if (e.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                ip = e.ip().toString();
                break;
            }
        }
        if (ip != QLatin1String("--"))
            break;
    }
    if (ip != m_ip) {
        m_ip = ip;
        emit ipAddressChanged();
    }
}