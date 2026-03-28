#include "../Inc/mcap_logger.h"
#include "../Inc/data_manager.h"
#include <thread>
#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <vector>

// Foxglove SDK includes
// NOTE: You need to install Foxglove SDK from https://github.com/foxglove/foxglove-sdk
// and link against the foxglove library in your CMakeLists.txt
#ifdef USE_FOXGLOVE_SDK
#include <foxglove/server.hpp>
#include <foxglove/mcap_writer.hpp>
#include <foxglove/channel.hpp>
#include <foxglove/message.hpp>
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

#ifdef USE_FOXGLOVE_SDK
// Foxglove SDK implementation
static void foxgloveLoggerLoop() {
    std::cout << "MCAP Logger: Initializing Foxglove SDK..." << std::endl;

    // Vector to hold sinks (MCAP writer and/or WebSocket server)
    std::vector<std::shared_ptr<foxglove::Sink>> sinks;

    // Create MCAP writer if output path is specified
    std::shared_ptr<foxglove::McapWriter> mcap_writer;
    if (!config.output_file_path.empty()) {
        auto writer_result = foxglove::McapWriter::create(config.output_file_path);
        if (!writer_result.has_value()) {
            std::cerr << "MCAP Logger: Failed to create MCAP writer: "
                      << foxglove::strerror(writer_result.error()) << std::endl;
        } else {
            mcap_writer = std::move(writer_result.value());
            sinks.push_back(mcap_writer);
            std::cout << "MCAP Logger: Writing to file " << config.output_file_path << std::endl;
        }
    }

    // Create WebSocket server if enabled
    std::shared_ptr<foxglove::WebSocketServer> ws_server;
    if (config.enable_websocket) {
        foxglove::WebSocketServerOptions ws_options;
        ws_options.host = config.websocket_host;
        ws_options.port = config.websocket_port;

        auto server_result = foxglove::WebSocketServer::create(std::move(ws_options));
        if (!server_result.has_value()) {
            std::cerr << "MCAP Logger: Failed to create WebSocket server: "
                      << foxglove::strerror(server_result.error()) << std::endl;
        } else {
            ws_server = std::move(server_result.value());
            sinks.push_back(ws_server);
            std::cout << "MCAP Logger: WebSocket server listening on "
                      << config.websocket_host << ":" << config.websocket_port << std::endl;
            std::cout << "MCAP Logger: Connect Foxglove to ws://"
                      << config.websocket_host << ":" << config.websocket_port << std::endl;
        }
    }

    if (sinks.empty()) {
        std::cerr << "MCAP Logger: No sinks configured, exiting" << std::endl;
        return;
    }

    // Create channel for protobuf messages
    foxglove::ChannelOptions channel_opts;
    channel_opts.topic = "/vehicle/telemetry";
    channel_opts.encoding = "protobuf";
    channel_opts.schema_name = "test.Data";
    // TODO: Add actual protobuf schema definition
    channel_opts.schema = ""; // Protobuf FileDescriptorSet would go here

    std::shared_ptr<foxglove::Channel> channel;
    for (auto& sink : sinks) {
        auto channel_result = sink->advertise_channel(channel_opts);
        if (!channel_result.has_value()) {
            std::cerr << "MCAP Logger: Failed to advertise channel: "
                      << foxglove::strerror(channel_result.error()) << std::endl;
            continue;
        }
        if (!channel) {
            channel = std::move(channel_result.value());
        }
    }

    if (!channel) {
        std::cerr << "MCAP Logger: Failed to create channel" << std::endl;
        return;
    }

    auto sleep_duration = std::chrono::milliseconds(1000 / config.sample_rate);
    std::cout << "MCAP Logger: Started at " << config.sample_rate << "Hz" << std::endl;

    while (logger_running.load()) {
        auto start = std::chrono::steady_clock::now();

        // Get latest data from DataManager
        test::Data data = DataManager::getInstance().getLatestData();

        // Serialize protobuf message
        std::string serialized;
        if (data.SerializeToString(&serialized)) {
            // Get current timestamp in nanoseconds
            auto now = std::chrono::system_clock::now();
            auto timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                now.time_since_epoch()).count();

            // Create message
            foxglove::Message msg;
            msg.timestamp = static_cast<uint64_t>(timestamp_ns);
            msg.data = std::vector<uint8_t>(serialized.begin(), serialized.end());

            // Publish to all sinks
            for (auto& sink : sinks) {
                auto result = sink->publish_message(channel, msg);
                if (!result.has_value()) {
                    std::cerr << "MCAP Logger: Failed to publish message: "
                              << foxglove::strerror(result.error()) << std::endl;
                }
            }
        }

        // Sleep for the remainder of the sample period
        auto elapsed = std::chrono::steady_clock::now() - start;
        auto remaining = sleep_duration - elapsed;
        if (remaining > std::chrono::milliseconds(0)) {
            std::this_thread::sleep_for(remaining);
        }
    }

    // Cleanup
    for (auto& sink : sinks) {
        sink->unadvertise_channel(channel);
    }

    std::cout << "MCAP Logger: Stopped" << std::endl;
}

#else
// Fallback implementation without Foxglove SDK
static void fallbackLoggerLoop() {
    std::cout << "MCAP Logger: Foxglove SDK not available, using fallback implementation" << std::endl;
    std::cout << "MCAP Logger: To enable Foxglove WebSocket streaming, build with -DUSE_FOXGLOVE_SDK" << std::endl;

    // Basic implementation without Foxglove SDK
    if (config.output_file_path.empty()) {
        std::cerr << "MCAP Logger: No output file specified and Foxglove SDK not available" << std::endl;
        return;
    }

    std::ofstream mcap_file(config.output_file_path, std::ios::binary);
    if (!mcap_file.is_open()) {
        std::cerr << "MCAP Logger: Failed to open " << config.output_file_path << std::endl;
        return;
    }

    std::cout << "MCAP Logger: Writing to " << config.output_file_path
              << " at " << config.sample_rate << "Hz" << std::endl;

    auto sleep_duration = std::chrono::milliseconds(1000 / config.sample_rate);

    while (logger_running.load()) {
        auto start = std::chrono::steady_clock::now();

        // Get latest data from DataManager
        test::Data data = DataManager::getInstance().getLatestData();

        // Serialize protobuf directly (simple format, not proper MCAP)
        std::string serialized;
        if (data.SerializeToString(&serialized)) {
            uint32_t length = serialized.size();
            mcap_file.write(reinterpret_cast<const char*>(&length), sizeof(length));
            mcap_file.write(serialized.data(), serialized.size());
        }

        // Sleep for the remainder of the sample period
        auto elapsed = std::chrono::steady_clock::now() - start;
        auto remaining = sleep_duration - elapsed;
        if (remaining > std::chrono::milliseconds(0)) {
            std::this_thread::sleep_for(remaining);
        }
    }

    mcap_file.close();
    std::cout << "MCAP Logger: Stopped" << std::endl;
}
#endif

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
    config.sample_rate = sample_rate_hz;
    config.enable_websocket = enable_websocket;
    config.websocket_host = websocket_host;
    config.websocket_port = websocket_port;

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
