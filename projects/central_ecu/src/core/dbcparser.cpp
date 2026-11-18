#include "dbcparser.h"
#include <QCanDbcFileParser>
#include <QCanMessageDescription>
#include <QCanUniqueIdDescription>
#include <QDateTime>
#include <QDebug>

DbcParser::DbcParser(QObject *parent)
    : QObject(parent){}

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

    m_topics.clear();
    for (const auto &msg : messages)
        m_topics.insert(msg.uniqueId(), QStringLiteral("/") + msg.name());

    m_processor.setUniqueIdDescription(QCanDbcFileParser::uniqueIdDescription());
    m_processor.setMessageDescriptions(messages);
    return true;
}

void DbcParser::processFrames(const QList<QCanBusFrame> &frames)
{
    for (const QCanBusFrame &frame : frames) {
        const QCanFrameProcessor::ParseResult res = m_processor.parseFrame(frame);
        auto it = m_topics.constFind(res.uniqueId);
        if (it == m_topics.cend())
            continue;

        const auto ts = frame.timeStamp();
        quint64 ns = quint64(ts.seconds()) * 1000000000ull
                   + quint64(ts.microSeconds()) * 1000ull;
        if (ns == 0)
            ns = quint64(QDateTime::currentMSecsSinceEpoch()) * 1000000ull;

        emit sample(it.value(), res.signalValues, ns);
    }
}