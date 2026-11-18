#pragma once

#include <QObject>
#include <QHash>
#include <QVariantMap>
#include <QByteArray>
#include <memory>
#include <string>

#include <foxglove/websocket.hpp>
#include <foxglove/channel.hpp>
#include <foxglove/mcap.hpp>

class FoxgloveSink : public QObject
{
    Q_OBJECT

public:
    explicit FoxgloveSink(QObject *parent = nullptr) : QObject(parent) {}
    ~FoxgloveSink() override;

    void startServer(uint16_t port = 8765);

public slots:
    void publishSample(const QString &topic, const QVariantMap &values, quint64 timestampNs);
    void startMcapLogging(bool enable, const QString &logDir);

private:
    struct ChannelEntry {
        std::string schemaStorage;
        std::shared_ptr<foxglove::RawChannel> channel;
    };

    const ChannelEntry *ensureChannel(const QString &topic, const QVariantMap &values);

    QHash<QString, ChannelEntry> m_channels;
    QByteArray m_jsonScratch;

    std::unique_ptr<foxglove::WebSocketServer> m_server;
    std::unique_ptr<foxglove::McapWriter> m_writer;
};