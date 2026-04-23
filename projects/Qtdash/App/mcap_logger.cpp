#include "mcap_logger.h"
#include "data_manager.h"
#include "can_bridge.h"

#include <QThread>
#include <QDateTime>
#include <QDebug>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <queue>
#include <unordered_set>
#include <optional>
#include <string>

#include <google/protobuf/descriptor.pb.h>

// Generated protobuf headers
#include "baja.pb.h"
#include "foxglove/CompressedImage.pb.h"   // from foxglove-sdk/include or vendored
#include "foxglove/RawAudio.pb.h"

#ifdef USE_FOXGLOVE_SDK
#  include <foxglove/server.hpp>
#  include <foxglove/mcap.hpp>
#  include <foxglove/channel.hpp>
#endif

// ─── Wall-clock helper ────────────────────────────────────────────────────────

static uint64_t nowNs() {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count());
}

// ─── Schema serialisation helpers ────────────────────────────────────────────
// Collect a descriptor and all its transitive dependencies into a
// FileDescriptorSet, then serialise to a binary string for the MCAP schema.

template<typename ProtoT>
static std::string buildSchema() {
    const google::protobuf::Descriptor* desc = ProtoT::descriptor();
    if (!desc) return {};

    google::protobuf::FileDescriptorSet fd_set;
    std::queue<const google::protobuf::FileDescriptor*> to_visit;
    std::unordered_set<std::string> visited;

    to_visit.push(desc->file());
    while (!to_visit.empty()) {
        const auto* fd = to_visit.front();
        to_visit.pop();
        if (visited.count(fd->name())) continue;
        fd->CopyTo(fd_set.add_file());
        visited.insert(fd->name());
        for (int i = 0; i < fd->dependency_count(); ++i)
            to_visit.push(fd->dependency(i));
    }

    std::string out;
    fd_set.SerializeToString(&out);
    return out;
}

// ─── Private implementation ───────────────────────────────────────────────────

struct McapLoggerConfig {
    std::string output_path;
    int         sample_rate_hz {50};
    bool        enable_ws      {true};
    std::string ws_host        {"0.0.0.0"};
    int         ws_port        {8765};
};

class McapLoggerPrivate {
public:
    std::atomic<bool> running {false};
    McapLoggerConfig  cfg;

#ifdef USE_FOXGLOVE_SDK
    // Channels are created on the logger thread and used from multiple threads,
    // so they are wrapped in optional and reset under running==false ordering.
    std::optional<foxglove::RawChannel> telemetryChannel;
    std::optional<foxglove::RawChannel> cameraChannel;
    std::optional<foxglove::RawChannel> audioChannel;
    std::optional<foxglove::RawChannel> uartChannel;
#endif
};

// ─── Singleton ────────────────────────────────────────────────────────────────

McapLogger& McapLogger::instance() {
    static McapLogger inst;
    return inst;
}

McapLogger::McapLogger(QObject* parent)
    : QObject(parent), d_(new McapLoggerPrivate()) {}

McapLogger::~McapLogger() {
    stop();
    delete d_;
}

bool McapLogger::isRunning() const { return d_->running.load(); }

// ─── start / stop ─────────────────────────────────────────────────────────────

void McapLogger::start(const QString& output_path,
                       int            sample_rate_hz,
                       bool           enable_ws,
                       const QString& ws_host,
                       int            ws_port) {
    if (d_->running.load()) return;

    d_->cfg.output_path    = output_path.toStdString();
    d_->cfg.sample_rate_hz = sample_rate_hz;
    d_->cfg.enable_ws      = enable_ws;
    d_->cfg.ws_host        = ws_host.toStdString();
    d_->cfg.ws_port        = ws_port;

#ifdef USE_FOXGLOVE_SDK
    // Run the blocking logger loop on a dedicated QThread so the Qt event loop
    // (and therefore QML / UI) stays responsive.
    auto* thread = new QThread(this);

    connect(thread, &QThread::started, this, [this, thread]() {
        // ── WebSocket server ──────────────────────────────────────────────────
        std::optional<foxglove::WebSocketServer> server;
        if (d_->cfg.enable_ws) {
            foxglove::WebSocketServerOptions ws_opts;
            ws_opts.host = d_->cfg.ws_host;
            ws_opts.port = static_cast<uint16_t>(d_->cfg.ws_port);
            ws_opts.capabilities = {foxglove::WebSocketServerCapabilities::ClientPublish};
            ws_opts.supported_encodings = {"json"};

            // Handle Foxglove "Publish" panel commands (same logic as your original)
            ws_opts.callbacks.onMessageData = [](
                                                  uint32_t /*client_id*/,
                                                  uint32_t /*client_channel_id*/,
                                                  const std::byte* data,
                                                  size_t           data_len)
            {
                std::string payload(reinterpret_cast<const char*>(data), data_len);
                qDebug() << "MCAP: Foxglove publish:" << payload.c_str();

                if      (payload.find("SET_MODE_PROFILE0") != std::string::npos) sendCanCommand("SET_MODE_PROFILE0");
                else if (payload.find("SET_MODE_PROFILE1") != std::string::npos) sendCanCommand("SET_MODE_PROFILE1");
                else if (payload.find("SET_MODE_PROFILE2") != std::string::npos) sendCanCommand("SET_MODE_PROFILE2");
                else if (payload.find("SET_MODE_PROFILE3") != std::string::npos) sendCanCommand("SET_MODE_PROFILE3");
                else if (payload.find("CALIBRATE")         != std::string::npos) sendCanCommand("CALIBRATE");
                else qWarning("MCAP: Unknown command: %s", payload.c_str());
            };

            auto res = foxglove::WebSocketServer::create(std::move(ws_opts));
            if (res.has_value()) {
                server = std::move(res.value());
                qDebug() << "MCAP: WebSocket server on"
                         << d_->cfg.ws_host.c_str() << ":" << d_->cfg.ws_port;
            } else {
                qWarning("MCAP: Failed to create WebSocket server");
                emit errorOccurred("WebSocket server failed to start");
            }
        }

        // ── MCAP file writer ──────────────────────────────────────────────────
        std::optional<foxglove::McapWriter> writer;
        if (!d_->cfg.output_path.empty()) {
            std::error_code ec;
            std::filesystem::create_directories(
                std::filesystem::path(d_->cfg.output_path).parent_path(), ec);

            foxglove::McapWriterOptions mcap_opts;
            mcap_opts.path = d_->cfg.output_path;
            auto res = foxglove::McapWriter::create(std::move(mcap_opts));
            if (res.has_value()) {
                writer = std::move(res.value());
                qDebug() << "MCAP: Writing to" << d_->cfg.output_path.c_str();
            } else {
                qWarning("MCAP: Failed to open file: %s", d_->cfg.output_path.c_str());
                emit errorOccurred(QString("Cannot open MCAP file: %1")
                                       .arg(d_->cfg.output_path.c_str()));
            }
        }

        if (!server.has_value() && !writer.has_value()) {
            d_->running.store(false);
            thread->quit();
            return;
        }

        // ── Telemetry channel (ubcbaja::Data) ─────────────────────────────────
        {
            auto schema_data = buildSchema<ubcbaja::Data>();
            foxglove::Schema schema;
            schema.name     = "ubcbaja.Data";
            schema.encoding = "protobuf";
            schema.data     = reinterpret_cast<const std::byte*>(schema_data.data());
            schema.data_len = schema_data.size();

            auto res = foxglove::RawChannel::create("/vehicle/telemetry", "protobuf", std::move(schema));
            if (res.has_value()) d_->telemetryChannel = std::move(res.value());
        }

        // ── Camera channel (foxglove::CompressedImage) ─────────────────────────
        {
            auto schema_data = buildSchema<foxglove::CompressedImage>();
            foxglove::Schema schema;
            schema.name     = "foxglove.CompressedImage";
            schema.encoding = "protobuf";
            schema.data     = reinterpret_cast<const std::byte*>(schema_data.data());
            schema.data_len = schema_data.size();

            auto res = foxglove::RawChannel::create("/camera/front", "protobuf", std::move(schema));
            if (res.has_value()) d_->cameraChannel = std::move(res.value());
        }

        // ── Audio channel (foxglove::RawAudio) ────────────────────────────────
        {
            auto schema_data = buildSchema<foxglove::RawAudio>();
            foxglove::Schema schema;
            schema.name     = "foxglove.RawAudio";
            schema.encoding = "protobuf";
            schema.data     = reinterpret_cast<const std::byte*>(schema_data.data());
            schema.data_len = schema_data.size();

            auto res = foxglove::RawChannel::create("/sensors/audio", "protobuf", std::move(schema));
            if (res.has_value()) d_->audioChannel = std::move(res.value());
        }

        // ── UART channel (ubcbaja::UartMessage) ───────────────────────────────
        {
            auto schema_data = buildSchema<ubcbaja::UartMessage>();
            foxglove::Schema schema;
            schema.name     = "ubcbaja.UartMessage";
            schema.encoding = "protobuf";
            schema.data     = reinterpret_cast<const std::byte*>(schema_data.data());
            schema.data_len = schema_data.size();

            auto res = foxglove::RawChannel::create("/sensors/uart", "protobuf", std::move(schema));
            if (res.has_value()) d_->uartChannel = std::move(res.value());
        }

        d_->running.store(true);
        emit loggerStarted();

        // ── Main telemetry loop ───────────────────────────────────────────────
        const auto interval = std::chrono::milliseconds(1000 / d_->cfg.sample_rate_hz);
        while (d_->running.load()) {
            auto tick_start = std::chrono::steady_clock::now();

            if (d_->telemetryChannel.has_value()) {
                ubcbaja::Data snapshot = DataManager::getInstance().getLatestData();
                std::string serialized;
                if (snapshot.SerializeToString(&serialized)) {
                    d_->telemetryChannel->log(
                        reinterpret_cast<const std::byte*>(serialized.data()),
                        serialized.size());
                }
            }

            auto elapsed   = std::chrono::steady_clock::now() - tick_start;
            auto remaining = interval - elapsed;
            if (remaining > std::chrono::milliseconds(0))
                QThread::msleep(static_cast<unsigned long>(
                    std::chrono::duration_cast<std::chrono::milliseconds>(remaining).count()));
        }

        // ── Teardown ──────────────────────────────────────────────────────────
        d_->telemetryChannel.reset();
        d_->cameraChannel.reset();
        d_->audioChannel.reset();
        d_->uartChannel.reset();

        if (writer.has_value()) writer->close();
        if (server.has_value()) server->stop();

        emit loggerStopped();
        thread->quit();
    }, Qt::DirectConnection);

    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();

#else
    qWarning("MCAP: Built without USE_FOXGLOVE_SDK — logger is a no-op");
    emit errorOccurred("Foxglove SDK not compiled in (missing USE_FOXGLOVE_SDK)");
#endif
}

void McapLogger::stop() {
    if (d_->running.exchange(false)) {
        // The loop checks running each iteration; it will exit and quit() the thread.
        qDebug() << "MCAP: Stop requested";
    }
}

// ─── Camera frame ─────────────────────────────────────────────────────────────

void McapLogger::logCameraFrame(const uint8_t* jpeg_data, size_t jpeg_size,
                                uint64_t timestamp_ns,
                                int /*width*/, int /*height*/) {
#ifdef USE_FOXGLOVE_SDK
    if (!d_->cameraChannel.has_value()) return;

    foxglove::CompressedImage img;
    auto* ts = img.mutable_timestamp();
    ts->set_seconds(static_cast<int64_t>(timestamp_ns / 1'000'000'000ULL));
    ts->set_nanos  (static_cast<int32_t>(timestamp_ns % 1'000'000'000ULL));
    img.set_frame_id("camera_front");
    img.set_format("jpeg");
    img.set_data(jpeg_data, jpeg_size);

    std::string serialized;
    if (img.SerializeToString(&serialized)) {
        d_->cameraChannel->log(
            reinterpret_cast<const std::byte*>(serialized.data()),
            serialized.size());
    }
#else
    Q_UNUSED(jpeg_data); Q_UNUSED(jpeg_size);
    Q_UNUSED(timestamp_ns);
#endif
}

// ─── Audio frame ──────────────────────────────────────────────────────────────

void McapLogger::logAudioFrame(const uint8_t* pcm_data, size_t pcm_size,
                               uint32_t sample_rate,
                               uint32_t num_channels,
                               uint32_t bits_per_sample) {
#ifdef USE_FOXGLOVE_SDK
    if (!d_->audioChannel.has_value()) {
        qWarning("MCAP: Audio channel not ready");
        return;
    }

    foxglove::RawAudio audio;
    audio.set_sample_rate(sample_rate);
    audio.set_num_channels(num_channels);
    audio.set_sample_size(bits_per_sample);
    audio.set_data(pcm_data, pcm_size);

    // Timestamp this frame with current wall clock
    auto* ts = audio.mutable_timestamp();
    uint64_t now = nowNs();
    ts->set_seconds(static_cast<int64_t>(now / 1'000'000'000ULL));
    ts->set_nanos  (static_cast<int32_t>(now % 1'000'000'000ULL));

    std::string serialized;
    if (audio.SerializeToString(&serialized)) {
        d_->audioChannel->log(
            reinterpret_cast<const std::byte*>(serialized.data()),
            serialized.size());
    }
#else
    Q_UNUSED(pcm_data); Q_UNUSED(pcm_size);
    Q_UNUSED(sample_rate); Q_UNUSED(num_channels); Q_UNUSED(bits_per_sample);
#endif
}

// ─── UART message ─────────────────────────────────────────────────────────────

void McapLogger::logUartMessage(const QString& port_name,
                                const QString& raw_line,
                                uint64_t       timestamp_ns) {
#ifdef USE_FOXGLOVE_SDK
    if (!d_->uartChannel.has_value()) return;

    if (timestamp_ns == 0) timestamp_ns = nowNs();

    ubcbaja::UartMessage msg;
    msg.set_timestamp_ns(timestamp_ns);
    msg.set_source(port_name.toStdString());
    msg.set_raw_line(raw_line.toStdString());

    std::string serialized;
    if (msg.SerializeToString(&serialized)) {
        d_->uartChannel->log(
            reinterpret_cast<const std::byte*>(serialized.data()),
            serialized.size());
    }
#else
    Q_UNUSED(port_name); Q_UNUSED(raw_line); Q_UNUSED(timestamp_ns);
#endif
}