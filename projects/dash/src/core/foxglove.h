#pragma once

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QImage> // <-- ADDED for Webcam
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

    // <-- ADDED: Slot to receive frames from the Webcam
    void broadcastImage(const QString& topic, const QImage& image);

private:
    std::unique_ptr<foxglove::WebSocketServer> m_server;
    std::unique_ptr<foxglove::McapWriter> m_mcapWriter;
    std::map<QString, std::unique_ptr<foxglove::RawChannel>> m_channels;
};