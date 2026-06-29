#pragma once
#include <QObject>
#include <QJsonObject>
#include <QHash>
#include <QString>
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

    signals:
    void serverStarted();

private:
    std::shared_ptr<foxglove::WebSocketServer> m_server;
    std::shared_ptr<foxglove::McapWriter> m_writer;

    QHash<QString, std::shared_ptr<foxglove::RawChannel>> m_channels;
};