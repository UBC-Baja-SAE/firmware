#ifndef DASHBOARD_QT_MCAP_LOGGER_H
#define DASHBOARD_QT_MCAP_LOGGER_H

#include <string>

/**
 * @brief Start the MCAP logger with optional file writing and WebSocket streaming.
 * Uses Foxglove SDK for live data visualization and MCAP recording.
 *
 * @param output_path Path to the output MCAP file (empty string to disable file writing)
 * @param sample_rate_hz Sample rate in Hz (default 100Hz)
 * @param enable_websocket Enable Foxglove WebSocket server for live streaming (default true)
 * @param websocket_host WebSocket server host (default "0.0.0.0" for all interfaces)
 * @param websocket_port WebSocket server port (default 8765)
 */
void startMcapLogger(const std::string& output_path = "",
                    int sample_rate_hz = 100,
                    bool enable_websocket = true,
                    const std::string& websocket_host = "0.0.0.0",
                    int websocket_port = 8765);

/**
 * @brief Stop the MCAP logger thread and close all connections.
 */
void stopMcapLogger();

/**
 * @brief Check if MCAP logger is running.
 */
bool isMcapLoggerRunning();

#endif //DASHBOARD_QT_MCAP_LOGGER_H