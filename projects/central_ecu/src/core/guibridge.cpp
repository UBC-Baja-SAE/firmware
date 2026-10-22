#include "guibridge.h"

GuiBridge::GuiBridge(QObject *parent)
    : QObject(parent), m_values(this)
{
    m_timer.setInterval(16);   // 60 Hz
    connect(&m_timer, &QTimer::timeout, this, &GuiBridge::flush);
    m_timer.start();
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