#include "dbcparser.h"
#include <QCanDbcFileParser>
#include <QCanMessageDescription>
#include <QCanSignalDescription>
#include <QCanUniqueIdDescription>
#include <QDebug>
#include <QJsonValue>
#include <QDateTime>

DbcParser::DbcParser(QObject *parent)
    : QObject(parent)
{
}

bool DbcParser::loadDbcFiles(const QStringList &filePaths)
{
    QList<QCanMessageDescription> allMessages;
    QJsonObject foxgloveSchemas;
    m_messageNames.clear();

    for (const QString &filePath : filePaths) {
        QCanDbcFileParser parser;
        if (!parser.parse(filePath)) {
            qWarning() << "Failed to parse DBC:" << filePath << "-" << parser.errorString();
            continue;
        }

        const auto messages = parser.messageDescriptions();
        allMessages.append(messages);

        for (const auto &msg : messages) {
            m_messageNames[msg.uniqueId()] = msg.name();

            QJsonObject properties;
            for (const auto &sig : msg.signalDescriptions()) {
                QJsonObject propProps;
                propProps["type"] = "number";
                propProps["title"] = sig.name();
                properties[sig.name()] = propProps;
            }

            QJsonObject tsProps;
            tsProps["type"] = "object";
            QJsonObject tsFields;
            tsFields["sec"] = QJsonObject{{"type", "integer"}};
            tsFields["nsec"] = QJsonObject{{"type", "integer"}};
            tsProps["properties"] = tsFields;
            properties["timestamp"] = tsProps;



            QJsonObject schema;
            schema["type"] = "object";
            schema["properties"] = properties;

            foxgloveSchemas[msg.name()] = schema;
        }
    }

    if (allMessages.isEmpty()) {
        qWarning() << "No CAN messages loaded from any DBC files.";
        return false;
    }

    m_processor.setUniqueIdDescription(QCanDbcFileParser::uniqueIdDescription());
    m_processor.setMessageDescriptions(allMessages);

    emit foxgloveTopicsGenerated(foxgloveSchemas);
    return true;
}

void DbcParser::processFrame(const QCanBusFrame &frame)
{
    QCanFrameProcessor::ParseResult result = m_processor.parseFrame(frame);

    if (m_messageNames.contains(result.uniqueId)) {
        const QString topicName = m_messageNames.value(result.uniqueId);

        QJsonObject payload;

        qint64 currentMs = QDateTime::currentMSecsSinceEpoch();
        QJsonObject tsObj;
        tsObj["sec"] = currentMs / 1000;
        tsObj["nsec"] = (currentMs % 1000) * 1000000;
        payload["timestamp"] = tsObj;

        for (auto it = result.signalValues.cbegin(); it != result.signalValues.cend(); ++it) {
            payload[it.key()] = QJsonValue::fromVariant(it.value());
        }

        emit frameParsed(topicName, payload);
    }
}