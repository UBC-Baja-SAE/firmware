#include "foxglove.h"
#include <foxglove/websocket.hpp>
#include <foxglove/channel.hpp>
#include <foxglove/mcap.hpp>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QDebug>
#include <QDir>
#include <QDateTime>

FoxgloveSink::FoxgloveSink(QObject *parent) : QObject(parent) {}

FoxgloveSink::~FoxgloveSink() {
    m_channels.clear();
    m_writer.reset();
    m_server.reset();
}

void FoxgloveSink::startServer(uint16_t port) {
    foxglove::WebSocketServerOptions options;
    options.port = port;

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
            topicName.toStdString(),          // Topic Name
            "json",                           // Message Encoding format
            channelSchema                     // Schema object
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

void FoxgloveSink::broadcastPayload(const QString &topicName, const QJsonObject &payload) {
    if (!m_channels.contains(topicName)) {
        return;
    }

    QJsonDocument doc(payload);
    QByteArray payloadBytes = doc.toJson(QJsonDocument::Compact);

    auto channel = m_channels.value(topicName);

    channel->log(reinterpret_cast<const std::byte*>(payloadBytes.constData()), payloadBytes.size());
}

void FoxgloveSink::toggleServer(bool enable) {
    if (enable && !m_server) {
        startServer(8765);
        if (!m_cachedSchemas.isEmpty()) {
            registerTopics(m_cachedSchemas); // Restore topics
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
            registerTopics(m_cachedSchemas); // Restore topics
        }
    } else if (!enable && m_writer) {
        stopMcapRecording();
    }
}

void FoxgloveSink::stopServer() {
    m_channels.clear(); // Clear old channels tied to this instance
    m_server.reset();   // Destroys the server
    qInfo() << "WebSocket server stopped.";
}

void FoxgloveSink::stopMcapRecording() {
    m_channels.clear();
    m_writer.reset();   // Destroys the writer and flushes the file
    qInfo() << "MCAP recording stopped.";
}