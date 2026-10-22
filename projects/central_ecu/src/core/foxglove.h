#pragma once

#include <QObject>
#include <QCanFrameProcessor>
#include <QHash>
#include <QVariantMap>
#include <QByteArray>
#include <memory>

#include <foxglove/websocket.hpp>
#include <foxglove/channel.hpp>
#include <foxglove/mcap.hpp>

class FoxgloveSink : public QObject
{
    Q_OBJECT

public:
    explicit FoxgloveSink(QObject *parent = nullptr);
    ~FoxgloveSink() override;

    void startServer(uint16_t port = 8765);

public slots:
    void registerTopic(QtCanBus::UniqueId id, const QString &name, const QByteArray &schemaJson);
    void publishSignals(QtCanBus::UniqueId id, const QVariantMap &signalsMap, quint64 timestampNs);
    void setMcapLogging(bool enable, const QString &logDir);

private:
    struct ChannelEntry {
        std::string schemaStorage;
        std::shared_ptr<foxglove::RawChannel> channel;
    };

    QHash<QtCanBus::UniqueId, ChannelEntry> m_channels;
    QByteArray m_jsonScratch;

    std::unique_ptr<foxglove::WebSocketServer> m_server;
    std::unique_ptr<foxglove::McapWriter> m_writer;
};