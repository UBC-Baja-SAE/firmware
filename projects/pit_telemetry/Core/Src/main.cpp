#include "../Inc/data_manager.h"
#include "../Inc/nrf24_handler.h"
#include "../Inc/mcap_logger.h"
#include <csignal>
#include <iostream>
#include <chrono>
#include <thread>

// ─── Signal Handler ───────────────────────────────────────────────────────────

static void handleSignal(int) {
    std::cout << "\nPit Receiver: shutting down..." << std::endl;
    stopNrf24();
    stopMcapLogger();
    exit(0);
}

// ─── Entry Point ─────────────────────────────────────────────────────────────

int main() {
    std::cout << "Pit Receiver: starting..." << std::endl;

    std::signal(SIGINT,  handleSignal);
    std::signal(SIGTERM, handleSignal);

    // WebSocket only — no file
    startMcapLogger("", 100, true, "0.0.0.0", 8765);

    if (!startNrf24()) {
        std::cerr << "Pit Receiver: NRF24 init failed, exiting" << std::endl;
        stopMcapLogger();
        return 1;
    }

    std::cout << "Pit Receiver: running — connect Foxglove to ws://<pit-pi-ip>:8765" << std::endl;

    // Keep main thread alive — all work happens in NRF24 + logger threads
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}