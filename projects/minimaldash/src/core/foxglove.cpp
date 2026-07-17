#include "foxglove.h"
#include <foxglove/websocket.hpp>
#include <foxglove/channel.hpp>
#include <foxglove/mcap.hpp>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QDebug>
#include <QDir>
#include <QDateTime>
#include <QBuffer>

FoxgloveSink::FoxgloveSink(QObject *parent) : QObject(parent) {}

FoxgloveSink::~FoxgloveSink() {
    m_channels.clear();
    m_writer.reset();
    m_server.reset();
}

void FoxgloveSink::startServer(uint16_t port) {
    foxglove::WebSocketServerOptions options;
    options.port = port;
    options.host = "0.0.0.0";

    auto serverResult = foxglove::WebSocketServer::create(std::move(options));
    if (!serverResult.has_value()) {
        qWarning() << "Failed to start Websocket:" << QString::fromStdString(foxglove::strerror(serverResult.error()));
        return;
    }

    m_server = std::make_shared<foxglove::WebSocketServer>(std::move(serverResult.value()));
    qInfo() << "WebSocket server listening on port" << port;

    emit serverStarted();
}

void FoxgloveSink::startMcapRecording(const QString &logDirectory) {
    QDir dir(logDirectory);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QString filename = QDateTime::currentDateTime().toString("MMMM_dd_hh_mmAP") + ".mcap";
    QString fullPath = dir.absoluteFilePath(filename);

    std::string stdPath = fullPath.toStdString();

    foxglove::McapWriterOptions options;
    options.path = stdPath;

    auto writerResult = foxglove::McapWriter::create(options);
    if (!writerResult.has_value()) {
        qWarning() << "Failed to start MCAP Writer:"
                   << QString::fromStdString(foxglove::strerror(writerResult.error()));
        return;
    }

    m_writer = std::make_shared<foxglove::McapWriter>(std::move(writerResult.value()));

    qInfo() << "MCAP recording started at:" << fullPath;
}

void FoxgloveSink::registerTopics(const QJsonObject &schemas) {
    m_cachedSchemas = schemas;
    if (!m_server && !m_writer) {
        qWarning() << "Cannot register topics: Neither Server nor Logger is initialized!";
        return;
    }

    for (auto it = schemas.begin(); it != schemas.end(); ++it) {
        const QString topicName = "/" + it.key();

        QJsonDocument doc(it.value().toObject());
        std::string schemaStr = doc.toJson(QJsonDocument::Compact).toStdString();

        foxglove::Schema channelSchema;
        channelSchema.name = it.key().toStdString();
        channelSchema.encoding = "jsonschema";

        channelSchema.data = reinterpret_cast<const std::byte*>(schemaStr.data());
        channelSchema.data_len = schemaStr.size();

        auto channelResult = foxglove::RawChannel::create(
            topicName.toStdString(),
            "json",
            channelSchema
        );

        if (channelResult.has_value()) {
            auto channel = std::make_shared<foxglove::RawChannel>(std::move(channelResult.value()));
            m_channels[it.key()] = channel;

            qDebug() << "Registered Foxglove topic:" << topicName;
        } else {
            qWarning() << "Failed to create channel for" << topicName;
        }
    }
}

void FoxgloveSink::registerMediaTopic(const QString &topicName, const QString &schemaName) {
    if (!m_server && !m_writer) return;

    std::string schemaStr;

    if (schemaName == "foxglove.CompressedImage") {
        schemaStr = R"({
            "title": "foxglove.CompressedImage",
            "type": "object",
            "properties": {
                "timestamp": {
                    "type": "object",
                    "properties": {"sec": {"type": "integer"}, "nsec": {"type": "integer"}}
                },
                "frame_id": {"type": "string"},
                "data": {"type": "string", "contentEncoding": "base64"},
                "format": {"type": "string"}
            }
        })";
    } else if (schemaName == "foxglove.CompressedAudio") {
        schemaStr = R"({
            "title": "foxglove.CompressedAudio",
            "type": "object",
            "properties": {
                "timestamp": {
                    "type": "object",
                    "properties": {"sec": {"type": "integer"}, "nsec": {"type": "integer"}}
                },
                "frame_id": {"type": "string"},
                "data": {"type": "string", "contentEncoding": "base64"},
                "format": {"type": "string"},
                "sample_rate": {"type": "integer"},
                "channels": {"type": "integer"}
            }
        })";
    }

    foxglove::Schema channelSchema;
    channelSchema.name = schemaName.toStdString();
    channelSchema.encoding = "jsonschema";
    channelSchema.data = reinterpret_cast<const std::byte*>(schemaStr.data());
    channelSchema.data_len = schemaStr.size();

    auto channelResult = foxglove::RawChannel::create(topicName.toStdString(), "json", channelSchema);

    if (channelResult.has_value()) {
        m_channels[topicName] = std::make_shared<foxglove::RawChannel>(std::move(channelResult.value()));
        qInfo() << "Registered media topic:" << topicName << "as" << schemaName;
    } else {
        qWarning() << "Failed to create channel for" << topicName;
    }
}

void FoxgloveSink::broadcastPayload(const QString &topicName, const QJsonObject &payload) {
    if (!m_channels.contains(topicName)) {
        return;
    }

    QJsonDocument doc(payload);
    QByteArray payloadBytes = doc.toJson(QJsonDocument::Compact);

    auto channel = m_channels.value(topicName);

    channel->log(reinterpret_cast<const std::byte*>(payloadBytes.constData()), payloadBytes.size());
}

void FoxgloveSink::broadcastImage(const QString &topic, const QImage &image) {
    // Lazily register the topic if it's the first frame
    if (!m_channels.contains(topic)) {
        registerMediaTopic(topic, "foxglove.CompressedImage");
    }

    // Compress QImage to JPEG inline
    QByteArray imgData;
    QBuffer buffer(&imgData);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "JPEG", 80);

    QJsonObject payload;
    QJsonObject timestamp;
    qint64 ms = QDateTime::currentMSecsSinceEpoch();
    timestamp["sec"] = ms / 1000;
    timestamp["nsec"] = (ms % 1000) * 1000000;

    payload["timestamp"] = timestamp;
    payload["frame_id"] = "camera";
    payload["format"] = "jpeg";
    payload["data"] = QString(imgData.toBase64());

    QJsonDocument doc(payload);
    QByteArray payloadBytes = doc.toJson(QJsonDocument::Compact);

    m_channels.value(topic)->log(reinterpret_cast<const std::byte*>(payloadBytes.constData()), payloadBytes.size());
}

void FoxgloveSink::broadcastAudio(const QString &topic, const QByteArray &data, int sampleRate, int channels) {
    // Lazily register the topic if it's the first audio chunk
    if (!m_channels.contains(topic)) {
        registerMediaTopic(topic, "foxglove.CompressedAudio");
    }

    QJsonObject payload;
    QJsonObject timestamp;
    qint64 ms = QDateTime::currentMSecsSinceEpoch();
    timestamp["sec"] = ms / 1000;
    timestamp["nsec"] = (ms % 1000) * 1000000;

    payload["timestamp"] = timestamp;
    payload["frame_id"] = "microphone";
    payload["format"] = "pcm";
    payload["sample_rate"] = sampleRate;
    payload["channels"] = channels;
    payload["data"] = QString(data.toBase64());

    QJsonDocument doc(payload);
    QByteArray payloadBytes = doc.toJson(QJsonDocument::Compact);

    m_channels.value(topic)->log(reinterpret_cast<const std::byte*>(payloadBytes.constData()), payloadBytes.size());
}

void FoxgloveSink::toggleServer(bool enable) {
    if (enable && !m_server) {
        startServer(8765);
        if (!m_cachedSchemas.isEmpty()) {
            registerTopics(m_cachedSchemas);
        }
    } else if (!enable && m_server) {
        stopServer();
    }
}

void FoxgloveSink::toggleLogging(bool enable) {
    if (enable && !m_writer) {
        QString logDir = QCoreApplication::applicationDirPath() + "/logs";
        startMcapRecording(logDir);
        if (!m_cachedSchemas.isEmpty()) {
            registerTopics(m_cachedSchemas);
        }
    } else if (!enable && m_writer) {
        stopMcapRecording();
    }
}

void FoxgloveSink::stopServer() {
    m_channels.clear();
    m_server.reset();
    qInfo() << "WebSocket server stopped.";
}

void FoxgloveSink::stopMcapRecording() {
    m_channels.clear();
    m_writer.reset();
    qInfo() << "MCAP recording stopped.";
}