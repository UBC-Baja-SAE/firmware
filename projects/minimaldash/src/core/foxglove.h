#pragma once
#include <QObject>
#include <QJsonObject>
#include <QHash>
#include <QString>
#include <QImage> // ADDED
#include <memory>

namespace foxglove {
    class WebSocketServer;
    class McapWriter;
    class RawChannel;
}

class FoxgloveSink : public QObject
{
    Q_OBJECT
public:
    explicit FoxgloveSink(QObject *parent = nullptr);
    ~FoxgloveSink();

public slots:
    void startServer(uint16_t port = 8765);
    void startMcapRecording(const QString &logDirectory);
    void registerTopics(const QJsonObject &schemas);
    void broadcastPayload(const QString &topicName, const QJsonObject &payload);

    // --- ADDED MEDIA SLOTS ---
    void broadcastImage(const QString &topic, const QImage &image);
    void broadcastAudio(const QString &topic, const QByteArray &data, int sampleRate, int channels);

    void toggleServer(bool enable);
    void toggleLogging(bool enable);

    signals:
        void serverStarted();

private:
    void stopServer();
    void stopMcapRecording();

    // --- ADDED HELPER ---
    void registerMediaTopic(const QString &topicName, const QString &schemaName);

    std::shared_ptr<foxglove::WebSocketServer> m_server;
    std::shared_ptr<foxglove::McapWriter> m_writer;

    QJsonObject m_cachedSchemas;

    QHash<QString, std::shared_ptr<foxglove::RawChannel>> m_channels;
};