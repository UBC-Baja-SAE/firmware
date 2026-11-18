#include "foxglove.h"
#include <QDateTime>
#include <QDir>
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <cmath>

FoxgloveSink::~FoxgloveSink()
{
    m_channels.clear();
    m_writer.reset();
    m_server.reset();
}

void FoxgloveSink::startServer(uint16_t port)
{
    foxglove::WebSocketServerOptions opts;
    opts.port = port;
    opts.host = "0.0.0.0";

    auto res = foxglove::WebSocketServer::create(std::move(opts));
    if (res.has_value()) {
        m_server = std::make_unique<foxglove::WebSocketServer>(std::move(res.value()));
        qInfo() << "Foxglove WebSocket listening on port" << port;
    } else {
        qWarning() << "Foxglove server error:"
                   << QString::fromStdString(foxglove::strerror(res.error()));
    }
}

const FoxgloveSink::ChannelEntry *FoxgloveSink::ensureChannel(const QString &topic,
                                                              const QVariantMap &values)
{
    auto it = m_channels.constFind(topic);
    if (it != m_channels.cend())
        return it->channel ? &it.value() : nullptr;

    QJsonObject props;
    for (auto v = values.cbegin(); v != values.cend(); ++v)
        props[v.key()] = QJsonObject{{"type", "number"}};
    const QJsonObject schemaObj{{"type", "object"}, {"properties", props}};

    ChannelEntry entry;
    entry.schemaStorage = QJsonDocument(schemaObj).toJson(QJsonDocument::Compact).toStdString();

    const std::string nameStr = (topic.startsWith('/') ? topic.mid(1) : topic).toStdString();
    const std::string topicStr = topic.toStdString();

    foxglove::Schema schema;
    schema.name = nameStr;
    schema.encoding = "jsonschema";
    schema.data = reinterpret_cast<const std::byte*>(entry.schemaStorage.data());
    schema.data_len = entry.schemaStorage.size();

    auto res = foxglove::RawChannel::create(topicStr, "json", schema);
    if (res.has_value())
        entry.channel = std::make_shared<foxglove::RawChannel>(std::move(res.value()));
    else
        qWarning() << "Channel create failed for" << topic
                   << QString::fromStdString(foxglove::strerror(res.error()));

    auto ins = m_channels.insert(topic, std::move(entry));
    return ins->channel ? &ins.value() : nullptr;
}

void FoxgloveSink::publishSample(const QString &topic, const QVariantMap &values, quint64 ts)
{
    const ChannelEntry *entry = ensureChannel(topic, values);
    if (!entry)
        return;

    m_jsonScratch.clear();
    m_jsonScratch.append('{');
    bool first = true;
    for (auto v = values.cbegin(); v != values.cend(); ++v) {
        if (!first) m_jsonScratch.append(',');
        first = false;
        m_jsonScratch.append('"');
        m_jsonScratch.append(v.key().toUtf8());
        m_jsonScratch.append("\":");

        const double d = v.value().toDouble();
        if (std::isfinite(d))
            m_jsonScratch.append(QByteArray::number(d, 'g', 9));
        else
            m_jsonScratch.append("null");
    }
    m_jsonScratch.append('}');

    entry->channel->log(reinterpret_cast<const std::byte*>(m_jsonScratch.constData()),
                        m_jsonScratch.size(), ts);
}

void FoxgloveSink::startMcapLogging(bool enable, const QString &logDir)
{
    if (enable && !m_writer) {
        if (!QDir().mkpath(logDir)) {
            qWarning() << "Could not create log directory:" << logDir;
            return;
        }

        const QString path = QDir(logDir).absoluteFilePath(
            QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss") + ".mcap");
        const std::string pathStr = path.toStdString();

        foxglove::McapWriterOptions opts;
        opts.path = pathStr;

        auto res = foxglove::McapWriter::create(opts);
        if (res.has_value()) {
            m_writer = std::make_unique<foxglove::McapWriter>(std::move(res.value()));
            qInfo() << "MCAP logging started:" << path;
        } else {
            qWarning() << "MCAP create failed:"
                       << QString::fromStdString(foxglove::strerror(res.error()));
        }
    } else if (!enable && m_writer) {
        m_writer.reset();
        qInfo() << "MCAP logging stopped.";
    }
}