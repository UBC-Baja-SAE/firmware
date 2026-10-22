#pragma once

#include <QObject>
#include <QCanBusFrame>
#include <QCanFrameProcessor>
#include <QHash>
#include <QVariantMap>
#include <QByteArray>

class DbcParser : public QObject
{
    Q_OBJECT

public:
    explicit DbcParser(QObject *parent = nullptr);
    bool loadDbc(const QString &filePath);

public slots:
    void processFrames(const QList<QCanBusFrame> &frames);

    signals:
        void topicDiscovered(QtCanBus::UniqueId id, const QString &name, const QByteArray &schemaJson);
        void messageDecoded(QtCanBus::UniqueId id, const QVariantMap &signalValues, quint64 timestampNs);

private:
    QCanFrameProcessor m_processor;
    QHash<QtCanBus::UniqueId, QString> m_validIds;
};