#include "../Inc/mcap_logger.h"
#include "../Inc/data_manager.h"
#include <thread>
#include <atomic>
#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>
#include <optional>
#include <filesystem>
#include <google/protobuf/descriptor.pb.h>

#ifdef USE_FOXGLOVE_SDK
    #include <foxglove/server.hpp>
    #include <foxglove/mcap.hpp>
    #include <foxglove/channel.hpp>
#else
    #include <mcap/writer.hpp>
#endif

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

// ─── Protobuf Schema ──────────────────────────────────────────────────────────

static std::string getProtobufSchema() {
    const google::protobuf::Descriptor* descriptor = test::Data::descriptor();
    if (!descriptor) {
        std::cerr << "MCAP Logger: Failed to get Data descriptor" << std::endl;
        return "";
    }

    const google::protobuf::FileDescriptor* file_descriptor = descriptor->file();
    if (!file_descriptor) {
        std::cerr << "MCAP Logger: Failed to get file descriptor" << std::endl;
        return "";
    }

    google::protobuf::FileDescriptorSet fd_set;

    for (int i = 0; i < file_descriptor->dependency_count(); ++i) {
        const google::protobuf::FileDescriptor* dep = file_descriptor->dependency(i);
        dep->CopyTo(fd_set.add_file());
    }

    file_descriptor->CopyTo(fd_set.add_file());

    std::string serialized;
    if (!fd_set.SerializeToString(&serialized)) {
        std::cerr << "MCAP Logger: Failed to serialize FileDescriptorSet" << std::endl;
        return "";
    }

    return serialized;
}

#ifdef USE_FOXGLOVE_SDK
// ─── Foxglove SDK Implementation ─────────────────────────────────────────────

static void foxgloveLoggerLoop() {
    std::cout << "MCAP Logger: Initializing Foxglove SDK..." << std::endl;

    // 1. Initialize WebSocket Server
    std::optional<foxglove::WebSocketServer> server;
    if (config.enable_websocket) {
        foxglove::WebSocketServerOptions ws_options;
        ws_options.host = config.websocket_host;
        ws_options.port = static_cast<uint16_t>(config.websocket_port);

        auto server_res = foxglove::WebSocketServer::create(std::move(ws_options));
        if (server_res.has_value()) {
            server = std::move(server_res.value());
            std::cout << "MCAP Logger: WebSocket server listening on ws://"
                      << config.websocket_host << ":" << server->port() << std::endl;
        } else {
            std::cerr << "MCAP Logger: Failed to start WebSocket server" << std::endl;
        }
    }

    // 2. Initialize MCAP Writer
    std::optional<foxglove::McapWriter> writer;
    if (!config.output_file_path.empty()) {

        // Create output directory if it doesn't exist
        std::filesystem::path file_path(config.output_file_path);
        std::error_code ec;
        std::filesystem::create_directories(file_path.parent_path(), ec);
        if (ec) {
            std::cerr << "MCAP Logger: Failed to create directory '"
                      << file_path.parent_path() << "': " << ec.message() << std::endl;
        } else {
            foxglove::McapWriterOptions mcap_options;
            mcap_options.path = config.output_file_path;

            auto writer_res = foxglove::McapWriter::create(std::move(mcap_options));
            if (writer_res.has_value()) {
                writer = std::move(writer_res.value());
                std::cout << "MCAP Logger: Writing MCAP to " << config.output_file_path << std::endl;
            } else {
                std::cerr << "MCAP Logger: Failed to create MCAP writer at '"
                          << config.output_file_path << "' (error code: "
                          << static_cast<int>(writer_res.error()) << ")" << std::endl;
            }
        }
    }

    if (!server.has_value() && !writer.has_value()) {
        std::cerr << "MCAP Logger: No outputs configured, exiting" << std::endl;
        return;
    }

    // 3. Create Schema and Channel
    std::string schema_data = getProtobufSchema();

    foxglove::Schema schema;
    schema.name     = "test.Data";
    schema.encoding = "protobuf";
    schema.data     = reinterpret_cast<const std::byte*>(schema_data.data());
    schema.data_len = schema_data.size();

    auto channel_res = foxglove::RawChannel::create("/vehicle/telemetry", "protobuf", std::move(schema));
    if (!channel_res.has_value()) {
        std::cerr << "MCAP Logger: Failed to create channel" << std::endl;
        return;
    }
    auto channel = std::move(channel_res.value());

    std::cout << "MCAP Logger: Started at " << config.sample_rate << "Hz" << std::endl;
    auto sleep_duration = std::chrono::milliseconds(1000 / config.sample_rate);

    // 4. Main Logging Loop
    while (logger_running.load()) {
        auto start = std::chrono::steady_clock::now();

        test::Data data = DataManager::getInstance().getLatestData();

        std::string serialized;
        if (data.SerializeToString(&serialized)) {
            channel.log(reinterpret_cast<const std::byte*>(serialized.data()), serialized.size());
        }

        auto elapsed   = std::chrono::steady_clock::now() - start;
        auto remaining = sleep_duration - elapsed;
        if (remaining > std::chrono::milliseconds(0)) {
            std::this_thread::sleep_for(remaining);
        }
    }

    // 5. Finalize outputs
    if (writer.has_value()) {
        writer->close();
        std::cout << "MCAP Logger: File finalized at " << config.output_file_path << std::endl;
    }
    if (server.has_value()) {
        server->stop();
    }

    std::cout << "MCAP Logger: Stopped" << std::endl;
}

#else
// ─── Fallback MCAP Writer (no WebSocket) ─────────────────────────────────────

static void fallbackLoggerLoop() {
    std::cout << "MCAP Logger: Foxglove SDK not available, using fallback MCAP writer." << std::endl;

    if (config.output_file_path.empty()) {
        std::cerr << "MCAP Logger: No output file specified." << std::endl;
        return;
    }

    // Create output directory if it doesn't exist
    std::filesystem::path file_path(config.output_file_path);
    std::error_code ec;
    std::filesystem::create_directories(file_path.parent_path(), ec);
    if (ec) {
        std::cerr << "MCAP Logger: Failed to create directory '"
                  << file_path.parent_path() << "': " << ec.message() << std::endl;
        return;
    }

    mcap::McapWriter writer;
    mcap::McapWriterOptions options("");

    auto status = writer.open(config.output_file_path, options);
    if (!status.ok()) {
        std::cerr << "MCAP Logger: Failed to open '" << config.output_file_path
                  << "': " << status.message << std::endl;
        return;
    }

    mcap::Schema schema("test.Data", "protobuf", getProtobufSchema());
    writer.addSchema(schema);

    mcap::Channel channel("/vehicle/telemetry", "protobuf", schema.id);
    writer.addChannel(channel);

    std::cout << "MCAP Logger: Writing MCAP to " << config.output_file_path
              << " at " << config.sample_rate << "Hz" << std::endl;

    auto sleep_duration  = std::chrono::milliseconds(1000 / config.sample_rate);
    uint32_t sequence_no = 0;

    while (logger_running.load()) {
        auto start = std::chrono::steady_clock::now();

        test::Data data = DataManager::getInstance().getLatestData();

        std::string serialized;
        if (data.SerializeToString(&serialized)) {
            auto     now          = std::chrono::system_clock::now();
            uint64_t timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                        now.time_since_epoch()).count();

            mcap::Message msg;
            msg.channelId  = channel.id;
            msg.sequence   = sequence_no++;
            msg.logTime    = timestamp_ns;
            msg.publishTime = timestamp_ns;
            msg.data       = reinterpret_cast<const std::byte*>(serialized.data());
            msg.dataSize   = serialized.size();

            writer.write(msg);
        }

        auto elapsed   = std::chrono::steady_clock::now() - start;
        auto remaining = sleep_duration - elapsed;
        if (remaining > std::chrono::milliseconds(0)) {
            std::this_thread::sleep_for(remaining);
        }
    }

    writer.close();
    std::cout << "MCAP Logger: Stopped and finalized " << config.output_file_path << std::endl;
}
#endif

// ─── Public API ───────────────────────────────────────────────────────────────

void startMcapLogger(const std::string& output_path,
                     int sample_rate_hz,
                     bool enable_websocket,
                     const std::string& websocket_host,
                     int websocket_port) {
    if (logger_running.load()) {
        std::cerr << "MCAP Logger: Already running" << std::endl;
        return;
    }

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
        if (logger_thread.joinable()) {
            logger_thread.join();
        }
    }
}

bool isMcapLoggerRunning() {
    return logger_running.load();
}