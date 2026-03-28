#include "../Inc/can_processor.h"
#include "../Inc/data_manager.h"
#include "../Inc/can_bridge.h"
#include "../Inc/can_id.h"
#include <thread>
#include <atomic>
#include <chrono>
#include <cstring>

static std::atomic<bool> processor_running(false);
static std::thread processor_thread;

// Helper to extract 6-byte IMU data into 3 floats (x, y, z)
static void extractImuData(uint64_t raw, float& x, float& y, float& z) {
    uint16_t raw_x, raw_y, raw_z;
    memcpy(&raw_x, &raw, 2);
    memcpy(&raw_y, ((uint8_t*)&raw) + 2, 2);
    memcpy(&raw_z, ((uint8_t*)&raw) + 4, 2);

    // Convert to float (adjust scale if needed)
    x = static_cast<float>(raw_x);
    y = static_cast<float>(raw_y);
    z = static_cast<float>(raw_z);
}

void processIncomingCanFrame(int can_id, uint8_t* data) {
    auto& dm = DataManager::getInstance();

    // --- Dashboard Sensors ---
    if (can_id == tachometer_id) {
        uint32_t rpm;
        memcpy(&rpm, data, 4);
        dm.setTach(rpm);
    }
    else if (can_id == speedometer_id) {
        uint32_t speed;
        memcpy(&speed, data, 4);
        dm.setSpeed(speed);
    }
    else if (can_id == thermometer_id) {
        uint32_t temp;
        memcpy(&temp, data, 4);
        dm.setTemp(temp);
    }
    else if (can_id == fuel_sensor_id) {
        uint32_t fuel;
        memcpy(&fuel, data, 4);
        dm.setFuel(fuel);
    }

    // --- Front Left ECU ---
    else if (can_id == fl_suspension) {
        uint16_t travel;
        memcpy(&travel, data, 2);
        dm.setEcuTravel(DataManager::FRONT_LEFT, static_cast<float>(travel));
    }
    else if (can_id == fl_strain_l_id || can_id == fl_strain_r_id) {
        uint16_t strain;
        memcpy(&strain, data, 2);
        dm.setEcuStrain(DataManager::FRONT_LEFT,
                       can_id == fl_strain_l_id ? static_cast<float>(strain) : 0.0f,
                       can_id == fl_strain_r_id ? static_cast<float>(strain) : 0.0f);
    }
    else if (can_id == fl_imu_accel_id) {
        float x, y, z;
        uint64_t raw = observed_data[can_id];
        extractImuData(raw, x, y, z);
        dm.setEcuAccel(DataManager::FRONT_LEFT, x, y, z);
    }

    // --- Front Right ECU ---
    else if (can_id == fr_suspension) {
        uint16_t travel;
        memcpy(&travel, data, 2);
        dm.setEcuTravel(DataManager::FRONT_RIGHT, static_cast<float>(travel));
    }
    else if (can_id == fr_strain_l_id || can_id == fr_strain_r_id) {
        uint16_t strain;
        memcpy(&strain, data, 2);
        dm.setEcuStrain(DataManager::FRONT_RIGHT,
                       can_id == fr_strain_l_id ? static_cast<float>(strain) : 0.0f,
                       can_id == fr_strain_r_id ? static_cast<float>(strain) : 0.0f);
    }
    else if (can_id == fr_imu_accel_id) {
        float x, y, z;
        uint64_t raw = observed_data[can_id];
        extractImuData(raw, x, y, z);
        dm.setEcuAccel(DataManager::FRONT_RIGHT, x, y, z);
    }

    // --- Rear Left ECU ---
    else if (can_id == rl_suspension) {
        uint16_t travel;
        memcpy(&travel, data, 2);
        dm.setEcuTravel(DataManager::REAR_LEFT, static_cast<float>(travel));
    }
    else if (can_id == rl_strain_l_id || can_id == rl_strain_r_id) {
        uint16_t strain;
        memcpy(&strain, data, 2);
        dm.setEcuStrain(DataManager::REAR_LEFT,
                       can_id == rl_strain_l_id ? static_cast<float>(strain) : 0.0f,
                       can_id == rl_strain_r_id ? static_cast<float>(strain) : 0.0f);
    }
    else if (can_id == rl_imu_accel_id) {
        float x, y, z;
        uint64_t raw = observed_data[can_id];
        extractImuData(raw, x, y, z);
        dm.setEcuAccel(DataManager::REAR_LEFT, x, y, z);
    }

    // --- Rear Right ECU ---
    else if (can_id == rr_suspension) {
        uint16_t travel;
        memcpy(&travel, data, 2);
        dm.setEcuTravel(DataManager::REAR_RIGHT, static_cast<float>(travel));
    }
    else if (can_id == rr_strain_l_id || can_id == rr_strain_r_id) {
        uint16_t strain;
        memcpy(&strain, data, 2);
        dm.setEcuStrain(DataManager::REAR_RIGHT,
                       can_id == rr_strain_l_id ? static_cast<float>(strain) : 0.0f,
                       can_id == rr_strain_r_id ? static_cast<float>(strain) : 0.0f);
    }
    else if (can_id == rr_imu_accel_id) {
        float x, y, z;
        uint64_t raw = observed_data[can_id];
        extractImuData(raw, x, y, z);
        dm.setEcuAccel(DataManager::REAR_RIGHT, x, y, z);
    }
}

static void processorLoop() {
    // Keep track of last processed values to detect changes
    uint64_t last_observed[2048] = {0};

    while (processor_running.load()) {
        // Poll at 100Hz
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        // Check all known CAN IDs for updates
        for (int id = 0; id < 2048; id++) {
            uint64_t current = observed_data[id];

            // If data changed, process it
            if (current != 0 && current != last_observed[id]) {
                uint8_t data[8];
                memcpy(data, &current, 8);
                processIncomingCanFrame(id, data);
                last_observed[id] = current;
            }
        }
    }
}

void startCanProcessor() {
    if (!processor_running.load()) {
        processor_running.store(true);
        processor_thread = std::thread(processorLoop);
        printf("CAN Processor: Started\n");
    }
}

void stopCanProcessor() {
    if (processor_running.load()) {
        processor_running.store(false);
        if (processor_thread.joinable()) {
            processor_thread.join();
        }
        printf("CAN Processor: Stopped\n");
    }
}