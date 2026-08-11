#pragma once
#include <QObject>
#include <QCanBusFrame>
#include <QCanFrameProcessor>
#include <QJsonObject>
#include <QHash>
#include <QStringList>

class DbcParser : public QObject
{
    Q_OBJECT
public:
    explicit DbcParser(QObject *parent = nullptr);

public slots:
    bool loadDbcFiles(const QStringList &filePath);

    void processFrame(const QCanBusFrame &frame);

    signals:
        void foxgloveTopicsGenerated(const QJsonObject &schemas);

    void frameParsed(const QString &topicName, const QJsonObject &payload);

private:
    QCanFrameProcessor m_processor;

    QHash<QtCanBus::UniqueId, QString> m_messageNames;
};