#pragma once

#include <QObject>
#include <QHash>
#include <QVariantMap>
#include <QByteArray>
#include <memory>
#include <string>
#include <optional>

#include <foxglove/websocket.hpp>
#include <foxglove/channel.hpp>
#include <foxglove/mcap.hpp>
#include <foxglove/messages.hpp>

class FoxgloveSink : public QObject
{
    Q_OBJECT

public:
    explicit FoxgloveSink(QObject *parent = nullptr) : QObject(parent) {}
    ~FoxgloveSink() override;

    void startServer(uint16_t port = 8765);

public slots:
    void publishSample(const QString &topic, const QVariantMap &values, quint64 timestampNs);
    void publishImage(const QByteArray &jpeg, quint64 ts);
    void publishAudio(const QByteArray &pcm, quint64 ts, quint32 rate, quint32 channels);
    void startMcapLogging(bool enable, const QString &logDir);

private:
    struct ChannelEntry {
        std::string messagestorage;
        std::shared_ptr<foxglove::RawChannel> channel;
    };

    const ChannelEntry *ensureChannel(const QString &topic, const QVariantMap &values);

    QHash<QString, ChannelEntry> m_channels;
    QByteArray m_jsonScratch;

    std::optional<foxglove::messages::CompressedImageChannel> m_imageChannel;
    std::optional<foxglove::messages::RawAudioChannel> m_audioChannel;

    std::unique_ptr<foxglove::WebSocketServer> m_server;
    std::unique_ptr<foxglove::McapWriter> m_writer;
};