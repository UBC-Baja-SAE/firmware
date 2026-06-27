#define FOXGLOVE_HIDE_TEMPLATES

#include "foxglove.h"
#include <QDebug>
#include <QDateTime>
#include <QDir>
#include <QBuffer>        // <-- ADDED for Webcam JPEG compression
#include <QJsonObject>    // <-- ADDED for Webcam JSON schema
#include <QJsonDocument>  // <-- ADDED for Webcam JSON schema

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
        qWarning() << "[Foxglove] Failed to start Foxglove Server";
        return;
    }
    m_server = std::make_unique<foxglove::WebSocketServer>(std::move(serverResult.value()));
    qInfo() << "[Foxglove] Server started on ws://0.0.0.0:8765";

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
        qWarning() << "[Foxglove] Failed to start MCAP writer at" << QString::fromStdString(safePath);

        if (errno != 0) {
            qWarning() << "[Foxglove] Real system error:" << strerror(errno);
        } else {
            qWarning() << "[Foxglove] Library error:" << foxglove::strerror(writerResult.error());
        }
    } else {
        m_mcapWriter = std::make_unique<foxglove::McapWriter>(std::move(writerResult.value()));
        qInfo() << "[Foxglove] MCAP logging to" << QString::fromStdString(safePath);
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
            qWarning() << "[Foxglove] Failed to create Foxglove channel for topic" << topic;
            return;
        }

        auto emplaceResult = m_channels.emplace(topic, std::make_unique<foxglove::RawChannel>(std::move(channelResult.value())));
        it = emplaceResult.first;
        qInfo() << "[Foxglove] Created Foxglove channel:" << topic;
    }
    it->second->log(reinterpret_cast<const std::byte*>(jsonPayload.constData()), jsonPayload.size());
}

// <-- ADDED: Entire block below for Webcam support
void FoxgloveServer::broadcastImage(const QString& topic, const QImage& image) {
    auto it = m_channels.find(topic);
    if (it == m_channels.end()) {
        // Foxglove standard schema for compressed images
        static const std::string schemaStr = R"({
            "title": "foxglove.CompressedImage",
            "type": "object",
            "properties": {
                "timestamp": {"type": "object", "properties": {"sec": {"type": "integer"}, "nsec": {"type": "integer"}}},
                "frame_id": {"type": "string"},
                "data": {"type": "string", "contentEncoding": "base64"},
                "format": {"type": "string"}
            }
        })";

        foxglove::Schema schema;
        schema.name = "foxglove.CompressedImage"; // Must be exactly this for Foxglove UI
        schema.encoding = "jsonschema";
        schema.data = reinterpret_cast<const std::byte*>(schemaStr.data());
        schema.data_len = schemaStr.size();

        auto channelResult = foxglove::RawChannel::create(topic.toStdString(), "json", schema);
        if (!channelResult.has_value()) return;

        auto emplaceResult = m_channels.emplace(topic, std::make_unique<foxglove::RawChannel>(std::move(channelResult.value())));
        it = emplaceResult.first;
        qInfo() << "[Foxglove] Created Camera channel:" << topic;
    }

    // 1. Compress raw QImage to JPEG
    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    // 50 is the quality (0-100). Keep it low to save bandwidth!
    image.save(&buffer, "JPG", 50);

    // 2. Base64 Encode
    QString base64Data = QString::fromLatin1(ba.toBase64());

    // 3. Get accurate timestamp
    auto now = std::chrono::system_clock::now().time_since_epoch();
    auto sec = std::chrono::duration_cast<std::chrono::seconds>(now).count();
    auto nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count() % 1000000000;

    // 4. Build JSON Payload
    QJsonObject jsonObj;
    QJsonObject timestampObj;
    timestampObj["sec"] = static_cast<qint64>(sec);
    timestampObj["nsec"] = static_cast<qint64>(nsec);

    jsonObj["timestamp"] = timestampObj;
    jsonObj["frame_id"] = "dashcam";
    jsonObj["format"] = "jpeg";
    jsonObj["data"] = base64Data;

    QByteArray jsonPayload = QJsonDocument(jsonObj).toJson(QJsonDocument::Compact);

    it->second->log(reinterpret_cast<const std::byte*>(jsonPayload.constData()), jsonPayload.size());
}