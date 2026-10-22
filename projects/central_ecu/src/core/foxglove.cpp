#include "foxglove.h"
#include <QDateTime>
#include <QDir>
#include <QDebug>

FoxgloveSink::FoxgloveSink(QObject *parent)
    : QObject(parent)
{
    m_jsonScratch.reserve(1024);
}

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

void FoxgloveSink::registerTopic(QtCanBus::UniqueId id, const QString &name, const QByteArray &schemaJson)
{
    ChannelEntry entry;
    entry.schemaStorage = schemaJson.toStdString();

    foxglove::Schema schema;
    schema.name = name.toStdString();
    schema.encoding = "jsonschema";
    schema.data = reinterpret_cast<const std::byte*>(entry.schemaStorage.data());
    schema.data_len = entry.schemaStorage.size();

    std::string topic = "/" + schema.name;
    auto res = foxglove::RawChannel::create(topic, "json", schema);
    if (res.has_value()) {
        entry.channel = std::make_shared<foxglove::RawChannel>(std::move(res.value()));
        m_channels.insert(id, std::move(entry));
    }
}

void FoxgloveSink::publishSignals(QtCanBus::UniqueId id, const QVariantMap &signalsMap, quint64 timestampNs)
{
    auto it = m_channels.constFind(id);
    if (it == m_channels.cend() || !it->channel)
        return;

    m_jsonScratch.clear();
    m_jsonScratch.append('{');
    bool first = true;
    for (auto sig = signalsMap.cbegin(); sig != signalsMap.cend(); ++sig) {
        if (!first) m_jsonScratch.append(',');
        first = false;
        m_jsonScratch.append('"');
        m_jsonScratch.append(sig.key().toUtf8());
        m_jsonScratch.append("\":");
        m_jsonScratch.append(QByteArray::number(sig.value().toDouble(), 'g', 9));
    }
    m_jsonScratch.append('}');

    it->channel->log(
        reinterpret_cast<const std::byte*>(m_jsonScratch.constData()),
        m_jsonScratch.size(),
        timestampNs
    );
}

void FoxgloveSink::setMcapLogging(bool enable, const QString &logDir)
{
    if (enable && !m_writer) {
        QDir dir(logDir);
        if (!dir.exists()) dir.mkpath(".");

        QString path = dir.absoluteFilePath(
        QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss") + ".mcap"
        );

        foxglove::McapWriterOptions opts;
        opts.path = path.toStdString();

        auto res = foxglove::McapWriter::create(opts);
        if (res.has_value()) {
            m_writer = std::make_unique<foxglove::McapWriter>(std::move(res.value()));
            qInfo() << "MCAP logging started:" << path;
        }
    } else if (!enable && m_writer) {
        m_writer.reset();
        qInfo() << "MCAP logging stopped.";
    }
}