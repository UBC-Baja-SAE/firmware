#pragma once
#include <QObject>
#include <QQmlPropertyMap>
#include <QTimer>
#include <QVariantMap>
#include <QCanUniqueIdDescription>

class GuiBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQmlPropertyMap* values READ values CONSTANT)
public:
    explicit GuiBridge(QObject *parent = nullptr);
    QQmlPropertyMap *values() { return &m_values; }

public slots:
    void onDecoded(QtCanBus::UniqueId id, const QVariantMap &signalValues, quint64 timestampNs);

private:
    void flush();

    QQmlPropertyMap m_values;
    QVariantMap     m_pending;
    QTimer          m_timer;
};