#include "dbcparser.h"
#include <QCanDbcFileParser>
#include <QCanMessageDescription>
#include <QCanSignalDescription>
#include <QCanUniqueIdDescription>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>

DbcParser::DbcParser(QObject *parent)
    : QObject(parent)
{
}

bool DbcParser::loadDbc(const QString &filePath)
{
    QCanDbcFileParser parser;
    if (!parser.parse(filePath)) {
        qWarning() << "DBC parse error:" << parser.errorString();
        return false;
    }

    const auto messages = parser.messageDescriptions();
    if (messages.isEmpty())
        return false;

    m_validIds.clear();
    for (const auto &msg : messages) {
        m_validIds.insert(msg.uniqueId(), msg.name());

        QJsonObject props;
        for (const auto &sig : msg.signalDescriptions()) {
            props[sig.name()] = QJsonObject{
                {"type", "number"},
                {"title", sig.name()}
            };
        }

        props["timestamp"] = QJsonObject{
            {"type", "object"},
            {"properties", QJsonObject{
                {"sec",  QJsonObject{{"type", "integer"}}},
                {"nsec", QJsonObject{{"type", "integer"}}}
            }}
        };

        QJsonObject schema{{"type", "object"}, {"properties", props}};
        QByteArray schemaBytes = QJsonDocument(schema).toJson(QJsonDocument::Compact);

        emit topicDiscovered(msg.uniqueId(), msg.name(), schemaBytes);
    }

    m_processor.setUniqueIdDescription(QCanDbcFileParser::uniqueIdDescription());
    m_processor.setMessageDescriptions(messages);
    return true;
}

void DbcParser::processFrames(const QList<QCanBusFrame> &frames)
{
    for (const QCanBusFrame &frame : frames) {
        const QCanFrameProcessor::ParseResult res = m_processor.parseFrame(frame);
        if (!m_validIds.contains(res.uniqueId))
            continue;

        const auto ts = frame.timeStamp();
        quint64 ns = quint64(ts.seconds()) * 1000000000ull
                   + quint64(ts.microSeconds()) * 1000ull;
        if (ns == 0)
            ns = quint64(QDateTime::currentMSecsSinceEpoch()) * 1000000ull;

        emit messageDecoded(res.uniqueId, res.signalValues, ns);
    }
}