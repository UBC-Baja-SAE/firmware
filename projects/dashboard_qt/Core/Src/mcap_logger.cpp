#include "../Inc/mcap_logger.h"
#include "../Inc/data_manager.h"
#include "../Inc/can_bridge.h"
#include <thread>
#include <atomic>
#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>
#include <optional>
#include <filesystem>
#include <string>
#include <queue>
#include <unordered_set>
#include <google/protobuf/descriptor.pb.h>

#ifdef USE_FOXGLOVE_SDK
    #include <foxglove/server.hpp>
    #include <foxglove/mcap.hpp>
    #include <foxglove/channel.hpp>
#else
    #include <mcap/writer.hpp>
#endif

// Generated from your protobufs
#include "Core/Src/image.pb.h"
#include "Core/Src/audio.pb.h"

static std::atomic<bool> logger_running(false);
static std::thread logger_thread;

struct LoggerConfig {
    std::string output_file_path;
    int sample_rate;
    bool enable_websocket;
    std::string websocket_host;
    int websocket_port;
};

static LoggerConfig config;

#ifdef USE_FOXGLOVE_SDK
static std::optional<foxglove::RawChannel> camera_channel;
static std::optional<foxglove::RawChannel> audio_channel;
#endif

// ─── Telemetry Schema ─────────────────────────────────────────────────────────

static std::string getProtobufSchema() {
    const google::protobuf::Descriptor* descriptor = ubcbaja::Data::descriptor();
    if (!descriptor) return "";

    const google::protobuf::FileDescriptor* file_descriptor = descriptor->file();
    if (!file_descriptor) return "";

    google::protobuf::FileDescriptorSet fd_set;
    for (int i = 0; i < file_descriptor->dependency_count(); ++i)
        file_descriptor->dependency(i)->CopyTo(fd_set.add_file());
    file_descriptor->CopyTo(fd_set.add_file());

    std::string serialized;
    fd_set.SerializeToString(&serialized);
    return serialized;
}

// ─── CompressedImage Schema ───────────────────────────────────────────────────

static std::string getCompressedImageSchema() {
    const google::protobuf::Descriptor* descriptor = foxglove::CompressedImage::descriptor();
    if (!descriptor) return "";

    google::protobuf::FileDescriptorSet fd_set;
    std::queue<const google::protobuf::FileDescriptor*> to_add;
    std::unordered_set<std::string> added;

    to_add.push(descriptor->file());
    while (!to_add.empty()) {
        const auto* fd = to_add.front();
        to_add.pop();
        if (added.count(fd->name())) continue;
        fd->CopyTo(fd_set.add_file());
        added.insert(fd->name());
        for (int i = 0; i < fd->dependency_count(); i++)
            to_add.push(fd->dependency(i));
    }

    std::string serialized;
    fd_set.SerializeToString(&serialized);
    return serialized;
}

// ─── Audio Schema ─────────────────────────────────────────────────────────────

static std::string getAudioSchema() {
    const google::protobuf::Descriptor* descriptor = foxglove::RawAudio::descriptor();
    if (!descriptor) return "";

    google::protobuf::FileDescriptorSet fd_set;
    std::queue<const google::protobuf::FileDescriptor*> to_add;
    std::unordered_set<std::string> added;

    to_add.push(descriptor->file());
    while (!to_add.empty()) {
        const auto* fd = to_add.front();
        to_add.pop();
        if (added.count(fd->name())) continue;
        fd->CopyTo(fd_set.add_file());
        added.insert(fd->name());
        for (int i = 0; i < fd->dependency_count(); i++)
            to_add.push(fd->dependency(i));
    }

    std::string serialized;
    fd_set.SerializeToString(&serialized);
    return serialized;
}

#ifdef USE_FOXGLOVE_SDK

// ─── CAN Command Handler ──────────────────────────────────────────────────────
//
// Called by the Foxglove "Publish" panel when it sends a message on the
// /can/command topic. The payload is a plain UTF-8 JSON string, e.g.:
//
//   { "command": "SET_MODE_SPORT" }
//
// We parse the "command" field and dispatch the matching CAN frame.
// Add new commands here by extending the if/else chain below.

static void handleCanCommand(const std::byte* data, size_t data_len) {
    // Convert raw bytes to a std::string for easy parsing
    std::string payload(reinterpret_cast<const char*>(data), data_len);

    printf("MCAP: Received Foxglove publish command: %s\n", payload.c_str());

    // Simple substring-based dispatch — no JSON library needed for this use case.
    // Add new commands here as your CAN protocol grows.
    if (payload.find("SET_MODE_PROFILE1") != std::string::npos) {
        sendCanCommand("SET_MODE_PROFILE1");
    } else if (payload.find("SET_MODE_PROFILE2") != std::string::npos) {
        sendCanCommand("SET_MODE_PROFILE2");
    } else if (payload.find("SET_MODE_PROFILE3") != std::string::npos) {
        sendCanCommand("SET_MODE_PROFILE3");
    } else if (payload.find("CALIBRATE") != std::string::npos) {
        sendCanCommand("CALIBRATE");
    } else {
        fprintf(stderr, "MCAP: Unknown CAN command in payload: %s\n", payload.c_str());
    }
}

// ─── Foxglove SDK Implementation ─────────────────────────────────────────────

static void foxgloveLoggerLoop() {
    std::cout << "MCAP Logger: Initializing Foxglove SDK..." << std::endl;

    // ── WebSocket Server ──────────────────────────────────────────────────────
    std::optional<foxglove::WebSocketServer> server;
    if (config.enable_websocket) {
        foxglove::WebSocketServerOptions ws_options;
        ws_options.host = config.websocket_host;
        ws_options.port = static_cast<uint16_t>(config.websocket_port);

        // Enable the Foxglove "Publish" panel to send messages back to us
        ws_options.capabilities = {foxglove::WebSocketServerCapabilities::ClientPublish};
        ws_options.supported_encodings = {"json"};

        // onMessageData fires whenever the Foxglove Publish panel sends a message.
        // client_channel_id identifies which client-advertised channel it came from.
        ws_options.callbacks.onMessageData = [](
            uint32_t client_id         [[maybe_unused]],
            uint32_t client_channel_id [[maybe_unused]],
            const std::byte* data,
            size_t data_len)
        {
            handleCanCommand(data, data_len);
        };

        auto server_res = foxglove::WebSocketServer::create(std::move(ws_options));
        if (server_res.has_value()) {
            server = std::move(server_res.value());
        } else {
            fprintf(stderr, "MCAP: Failed to create WebSocket server\n");
        }
    }

    // ── MCAP File Writer ──────────────────────────────────────────────────────
    std::optional<foxglove::McapWriter> writer;
    if (!config.output_file_path.empty()) {
        std::filesystem::path file_path(config.output_file_path);
        std::error_code ec;
        std::filesystem::create_directories(file_path.parent_path(), ec);
        if (!ec) {
            foxglove::McapWriterOptions mcap_options;
            mcap_options.path = config.output_file_path;

            auto writer_res = foxglove::McapWriter::create(std::move(mcap_options));
            if (writer_res.has_value()) {
                writer = std::move(writer_res.value());
            }
        }
    }

    if (!server.has_value() && !writer.has_value()) return;

    // ── Telemetry Channel ─────────────────────────────────────────────────────
    std::string schema_data = getProtobufSchema();
    foxglove::Schema schema;
    schema.name     = "ubcbaja.Data";
    schema.encoding = "protobuf";
    schema.data     = reinterpret_cast<const std::byte*>(schema_data.data());
    schema.data_len = schema_data.size();

    auto channel_res = foxglove::RawChannel::create("/vehicle/telemetry", "protobuf", std::move(schema));
    if (!channel_res.has_value()) return;
    auto channel = std::move(channel_res.value());

    // ── Camera Channel ────────────────────────────────────────────────────────
    std::string cam_schema_data = getCompressedImageSchema();
    foxglove::Schema cam_schema;
    cam_schema.name     = "foxglove.CompressedImage";
    cam_schema.encoding = "protobuf";
    cam_schema.data     = reinterpret_cast<const std::byte*>(cam_schema_data.data());
    cam_schema.data_len = cam_schema_data.size();

    auto cam_channel_res = foxglove::RawChannel::create("/camera/front", "protobuf", std::move(cam_schema));
    if (cam_channel_res.has_value()) {
        camera_channel = std::move(cam_channel_res.value());
    }

    // ── Audio Channel ─────────────────────────────────────────────────────────
    std::string audio_schema_data = getAudioSchema();
    foxglove::Schema audio_schema;
    audio_schema.name     = "foxglove.RawAudio";
    audio_schema.encoding = "protobuf";
    audio_schema.data     = reinterpret_cast<const std::byte*>(audio_schema_data.data());
    audio_schema.data_len = audio_schema_data.size();

    auto audio_channel_res = foxglove::RawChannel::create("/camera/audio", "protobuf", std::move(audio_schema));
    if (audio_channel_res.has_value()) {
        audio_channel = std::move(audio_channel_res.value());
    }

    // ── Main Logging Loop ─────────────────────────────────────────────────────
    auto sleep_duration = std::chrono::milliseconds(1000 / config.sample_rate);

    while (logger_running.load()) {
        auto start = std::chrono::steady_clock::now();

        ubcbaja::Data data = DataManager::getInstance().getLatestData();

        std::string serialized;
        if (data.SerializeToString(&serialized)) {
            channel.log(reinterpret_cast<const std::byte*>(serialized.data()), serialized.size());
        }

        auto elapsed   = std::chrono::steady_clock::now() - start;
        auto remaining = sleep_duration - elapsed;
        if (remaining > std::chrono::milliseconds(0))
            std::this_thread::sleep_for(remaining);
    }

    camera_channel.reset();
    audio_channel.reset();
    if (writer.has_value()) writer->close();
    if (server.has_value()) server->stop();
}

#else
// ─── Fallback MCAP Writer ─────────────────────────────────────────────────────
static void fallbackLoggerLoop() {}
#endif

// ─── Camera Frame Logging ─────────────────────────────────────────────────────

void logCameraFrame(const uint8_t* jpeg_data, size_t jpeg_size,
                    uint64_t timestamp_ns, int width, int height) {
#ifdef USE_FOXGLOVE_SDK
    if (!camera_channel.has_value()) return;

    foxglove::CompressedImage img;
    auto* ts = img.mutable_timestamp();
    ts->set_seconds(static_cast<int64_t>(timestamp_ns / 1000000000ULL));
    ts->set_nanos(static_cast<int32_t>(timestamp_ns % 1000000000ULL));

    img.set_frame_id("camera_front");
    img.set_data(jpeg_data, jpeg_size);
    img.set_format("jpeg");

    std::string serialized;
    if (img.SerializeToString(&serialized)) {
        camera_channel->log(reinterpret_cast<const std::byte*>(serialized.data()), serialized.size());
    }
#else
    (void)jpeg_data; (void)jpeg_size; (void)timestamp_ns; (void)width; (void)height;
#endif
}

// ─── Audio Frame Logging ──────────────────────────────────────────────────────

void logAudioFrame(const uint8_t* data, size_t size) {
#ifdef USE_FOXGLOVE_SDK
    if (!audio_channel.has_value()) {
        printf("Audio Trace: ERROR - audio_channel is not ready!\n");
        return;
    }

    printf("Audio Trace: SUCCESS - Writing to Foxglove SDK\n");
    audio_channel->log(reinterpret_cast<const std::byte*>(data), size);
#else
    (void)data; (void)size;
#endif
}

// ─── Public API ───────────────────────────────────────────────────────────────

void startMcapLogger(const std::string& output_path, int sample_rate_hz,
                     bool enable_websocket, const std::string& websocket_host, int websocket_port) {
    if (logger_running.load()) return;

    config.output_file_path = output_path;
    config.sample_rate      = sample_rate_hz;
    config.enable_websocket = enable_websocket;
    config.websocket_host   = websocket_host;
    config.websocket_port   = websocket_port;

    logger_running.store(true);
#ifdef USE_FOXGLOVE_SDK
    logger_thread = std::thread(foxgloveLoggerLoop);
#else
    logger_thread = std::thread(fallbackLoggerLoop);
#endif
}

void stopMcapLogger() {
    if (logger_running.load()) {
        logger_running.store(false);
        if (logger_thread.joinable()) logger_thread.join();
    }
}

bool isMcapLoggerRunning() {
    return logger_running.load();
}