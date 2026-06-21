#pragma once

#include <QObject>
#include <QByteArray>
#include <memory>

// Forward declarations hide the Foxglove headers from Qt's MOC,
// completely bypassing the MSVC 'explicit specialization' compiler bug.
namespace foxglove {
    class WebSocketServer;
    class RawChannel;
}

class FoxgloveServer : public QObject {
    Q_OBJECT
public:
    explicit FoxgloveServer(QObject* parent = nullptr);
    ~FoxgloveServer();

    void start();
    void stop();

public slots:
    void broadcastCanFrame(const QByteArray& jsonPayload);

private:
    std::unique_ptr<foxglove::WebSocketServer> m_server;
    std::unique_ptr<foxglove::RawChannel> m_channel;
};