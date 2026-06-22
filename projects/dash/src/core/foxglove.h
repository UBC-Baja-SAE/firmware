#pragma once

#include <QObject>
#include <QByteArray>
#include <QString>
#include <map>
#include <memory>

namespace foxglove {
    class WebSocketServer;
    class RawChannel;
    class McapWriter;
}

class FoxgloveServer : public QObject {
    Q_OBJECT
public:
    explicit FoxgloveServer(QObject* parent = nullptr);
    ~FoxgloveServer();

    void start();
    void stop();

public slots:
    void broadcastCanFrame(const QString& topic, const QByteArray& jsonPayload);

private:
    std::unique_ptr<foxglove::WebSocketServer> m_server;
    std::unique_ptr<foxglove::McapWriter> m_mcapWriter;
    std::map<QString, std::unique_ptr<foxglove::RawChannel>> m_channels;
};