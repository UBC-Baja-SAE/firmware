#define FOXGLOVE_HIDE_TEMPLATES

#include "foxglove.h"
#include <QDebug>
#include <QDateTime>
#include <QDir>
#include <foxglove/websocket.hpp>
#include <foxglove/channel.hpp>
#include <foxglove/mcap.hpp>
#include <foxglove/error.hpp>
#include <cerrno>
#include <cstring>
#include <ctime>
#include <chrono>

FoxgloveServer::FoxgloveServer(QObject* parent) : QObject(parent) {}

FoxgloveServer::~FoxgloveServer() {
    stop();
}

void FoxgloveServer::start() {
    foxglove::WebSocketServerOptions options;
    options.host = "0.0.0.0";
    options.port = 8765;
    auto serverResult = foxglove::WebSocketServer::create(std::move(options));
    if (!serverResult.has_value()) {
        qWarning() << "Failed to start Foxglove Server";
        return;
    }
    m_server = std::make_unique<foxglove::WebSocketServer>(std::move(serverResult.value()));
    qInfo() << "Foxglove Server started on ws://0.0.0.0:8765";

    // MCAP logging setup
    const QString logDir = "/home/ubcbaja/firmware/logs";
    QDir().mkpath(logDir);

    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);

    char timeStr[100];
    std::strftime(timeStr, sizeof(timeStr), "Mochi_%B_%d_%H_%M", std::localtime(&now_c));

    foxglove::McapWriterOptions mcapOptions;

    std::string safePath = "/home/ubcbaja/firmware/logs/" + std::string(timeStr) + ".mcap";
    mcapOptions.path = safePath;

    errno = 0;

    auto writerResult = foxglove::McapWriter::create(mcapOptions);
    if (!writerResult.has_value()) {
        qWarning() << "Failed to start MCAP writer at" << QString::fromStdString(safePath);

        if (errno != 0) {
            qWarning() << "Real system error:" << strerror(errno);
        } else {
            qWarning() << "Library error:" << foxglove::strerror(writerResult.error());
        }
    } else {
        m_mcapWriter = std::make_unique<foxglove::McapWriter>(std::move(writerResult.value()));
        qInfo() << "MCAP logging to" << QString::fromStdString(safePath);
    }
}

void FoxgloveServer::stop() {
    m_channels.clear();
    if (m_mcapWriter) {
        m_mcapWriter->close();
        m_mcapWriter.reset();
    }
    if (m_server) {
        m_server->stop();
        m_server.reset();
    }
}

void FoxgloveServer::broadcastCanFrame(const QString& topic, const QByteArray& jsonPayload) {
    auto it = m_channels.find(topic);
    if (it == m_channels.end()) {
        static const std::string schemaStr = R"({ "title": "Can Frame", "type": "object" })";
        foxglove::Schema schema;
        schema.name = "Can Frame";
        schema.encoding = "jsonschema";
        schema.data = reinterpret_cast<const std::byte*>(schemaStr.data());
        schema.data_len = schemaStr.size();

        auto channelResult = foxglove::RawChannel::create(topic.toStdString(), "json", schema);
        if (!channelResult.has_value()) {
            qWarning() << "Failed to create Foxglove channel for topic" << topic;
            return;
        }

        auto emplaceResult = m_channels.emplace(topic, std::make_unique<foxglove::RawChannel>(std::move(channelResult.value())));
        it = emplaceResult.first;
        qInfo() << "Created Foxglove channel:" << topic;
    }
    it->second->log(reinterpret_cast<const std::byte*>(jsonPayload.constData()), jsonPayload.size());
}