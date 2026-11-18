#include "guibridge.h"
#include <utility>

GuiBridge::GuiBridge(QObject *parent)
    : QObject(parent), m_values(this)
{
    m_timer.setInterval(16);   // 60 Hz
    connect(&m_timer, &QTimer::timeout, this, &GuiBridge::flush);
    m_timer.start();
}

void GuiBridge::onSample(const QString &topic, const QVariantMap &values, quint64)
{
    const QString prefix = (topic.startsWith('/') ? topic.mid(1) : topic) + QLatin1Char('_');

    QMutexLocker lock(&m_mutex);
    for (auto it = values.cbegin(); it != values.cend(); ++it)
        m_pending.insert(prefix + it.key(), it.value());
}

void GuiBridge::flush()
{
    QVariantMap pending;
    {
        QMutexLocker lock(&m_mutex);
        if (m_pending.isEmpty()) return;
        pending.swap(m_pending);
    }
    for (auto it = pending.cbegin(); it != pending.cend(); ++it)
        m_values.insert(it.key(), it.value());
}