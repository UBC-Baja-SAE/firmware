#pragma once
#include <QObject>
#include <QQmlPropertyMap>
#include <QTimer>
#include <QVariantMap>
#include <QMutex>

class GuiBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQmlPropertyMap* values READ values CONSTANT)
public:
    explicit GuiBridge(QObject *parent = nullptr);
    QQmlPropertyMap *values() { return &m_values; }

public slots:
    void onSample(const QString &topic, const QVariantMap &values, quint64 timestampNs);

private:
    void flush();

    QQmlPropertyMap m_values;
    QVariantMap m_pending;
    QTimer m_timer;
    QMutex m_mutex;
};