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
        void topicDiscovered(QString topic, QString encoding, QString schemaName, QString schemaEncoding, QByteArray schema);
        void sample(QString topic, QVariantMap values, quint64 tsNs);


private:
    QCanFrameProcessor m_processor;
    QHash<QtCanBus::UniqueId, QString> m_topics;
};