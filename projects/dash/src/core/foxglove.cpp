#include "foxglove.h"
#include <QDebug>
#include <foxglove/websocket.hpp>
#include <foxglove/channel.hpp>

FoxgloveServer::FoxgloveServer(QObject* parent) : QObject(parent) {}

FoxgloveServer::~FoxgloveServer() {
    stop();
}

void FoxgloveServer::start() {
    foxglove::WebSocketServerOptions options;
    auto serverResult = foxglove::WebSocketServer::create(std::move(options));
    if (!serverResult.has_value()) {
        qWarning() << "Failed to start Foxglove Server";
        return;
    }
    m_server = std::make_unique<foxglove::WebSocketServer>(std::move(serverResult.value()));

    static const std::string schemaStr = R"({ "title": "CanFrame", "type": "object" })";

    foxglove::Schema schema;
    schema.name = "CanFrame";
    schema.encoding = "jsonschema";
    schema.data = reinterpret_cast<const std::byte*>(schemaStr.data());
    schema.data_len = schemaStr.size();

    auto channelResult = foxglove::RawChannel::create("/can_bus", "json", schema);
    if (!channelResult.has_value()) {
        qWarning() << "Failed to create Foxglove Channel";
        return;
    }
    m_channel = std::make_unique<foxglove::RawChannel>(std::move(channelResult.value()));
    qInfo() << "Foxglove Telemetry Server started on ws://127.0.0.1:8765";
}

void FoxgloveServer::stop() {
    m_channel.reset();
    if (m_server) {
        m_server->stop();
        m_server.reset();
    }
}

void FoxgloveServer::broadcastCanFrame(const QByteArray& jsonPayload) {
    if (!m_channel) return;
    m_channel->log(reinterpret_cast<const std::byte*>(jsonPayload.constData()), jsonPayload.size());
}