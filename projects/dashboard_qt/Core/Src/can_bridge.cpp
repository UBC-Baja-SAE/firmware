#include "../Inc/can_bridge.h"
#include "../Inc/can_id.h"
#include "../Inc/data_manager.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <thread>
#include <atomic>
#include <string>

#ifdef __linux__
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/socket.h>
#include <net/if.h>
#include <unistd.h>
#include <sys/ioctl.h>
#endif

// Legacy UI support
volatile uint64_t observed_data[2048] = {0};

static std::atomic<bool> running(false);
static std::thread can_thread;

#ifdef __linux__

// ─── Helpers ──────────────────────────────────────────────────────────────────

// Explicit ID mapping — safer than range checks given the sparse ID layout
static DataManager::EcuPosition getEcuPos(int id) {
    if (id == fl_imu_accel_id || id == fl_imu_gyro_id ||
        id == fl_suspension    || id == fl_strain_l_id || id == fl_strain_r_id)
        return DataManager::FRONT_LEFT;

    if (id == fr_imu_accel_id || id == fr_imu_gyro_id ||
        id == fr_suspension    || id == fr_strain_l_id || id == fr_strain_r_id)
        return DataManager::FRONT_RIGHT;

    if (id == rl_imu_accel_id || id == rl_imu_gyro_id ||
        id == rl_suspension    || id == rl_strain_l_id || id == rl_strain_r_id)
        return DataManager::REAR_LEFT;

    if (id == rr_imu_accel_id || id == rr_imu_gyro_id ||
        id == rr_suspension    || id == rr_strain_l_id || id == rr_strain_r_id)
        return DataManager::REAR_RIGHT;

    return DataManager::FRONT_LEFT; // fallback
}

// ─── CAN Reader Thread ────────────────────────────────────────────────────────

static void canReaderThread(std::string interface_name) {
    int s;
    struct sockaddr_can addr;
    struct ifreq ifr;

    if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
        perror("CAN Bridge: Socket Error");
        return;
    }

    strncpy(ifr.ifr_name, interface_name.c_str(), IFNAMSIZ - 1);
    if (ioctl(s, SIOCGIFINDEX, &ifr) < 0) {
        fprintf(stderr, "CAN Bridge: Interface '%s' not found\n", interface_name.c_str());
        close(s);
        return;
    }

    addr.can_family  = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("CAN Bridge: Bind Error");
        close(s);
        return;
    }

    printf("CAN Bridge: Listening on %s...\n", interface_name.c_str());

    auto& dm = DataManager::getInstance();
    struct can_frame frame;

    while (running.load()) {
        int nbytes = read(s, &frame, sizeof(struct can_frame));
        if (nbytes <= 0) continue;

        int id = frame.can_id & 0x7FF;
        if (id >= 2048) continue;

        // Mirror into legacy array
        uint64_t payload = 0;
        memcpy(&payload, frame.data, (frame.can_dlc > 8) ? 8 : frame.can_dlc);
        observed_data[id] = payload;

        // ── 1. Powertrain — 4-byte unsigned ──────────────────────────────────
        if (id == tachometer_id || id == speedometer_id ||
            id == thermometer_id || id == fuel_sensor_id) {

            uint32_t val;
            memcpy(&val, frame.data, 4);

            if      (id == tachometer_id)  dm.setTach(val);
            else if (id == speedometer_id) dm.setSpeed(val);
            else if (id == thermometer_id) dm.setTemp(val);
            else if (id == fuel_sensor_id) dm.setFuel(val);
        }

        // ── 2. Suspension travel — 2-byte unsigned ────────────────────────────
        else if (id == fl_suspension || id == fr_suspension ||
                 id == rl_suspension || id == rr_suspension) {

            uint16_t val;
            memcpy(&val, frame.data, 2);
            dm.setEcuTravel(getEcuPos(id), static_cast<float>(val));
        }

        // ── 3. Strain gauges — each side is its own frame, 2-byte signed ─────
        else if (id == fl_strain_l_id || id == fr_strain_l_id ||
                 id == rl_strain_l_id || id == rr_strain_l_id) {

            int16_t val;
            memcpy(&val, frame.data, 2);
            // Pass 0.0f for the side we don't have — DataManager ignores zeroes
            dm.setEcuStrain(getEcuPos(id), static_cast<float>(val), 0.0f);
        }

        else if (id == fl_strain_r_id || id == fr_strain_r_id ||
                 id == rl_strain_r_id || id == rr_strain_r_id) {

            int16_t val;
            memcpy(&val, frame.data, 2);
            dm.setEcuStrain(getEcuPos(id), 0.0f, static_cast<float>(val));
        }

        // ── 4. IMU accelerometer — 3x 2-byte signed ──────────────────────────
        else if (id == fl_imu_accel_id || id == fr_imu_accel_id ||
                 id == rl_imu_accel_id || id == rr_imu_accel_id) {

            int16_t x, y, z;
            memcpy(&x, &frame.data[0], 2);
            memcpy(&y, &frame.data[2], 2);
            memcpy(&z, &frame.data[4], 2);
            dm.setEcuAccel(getEcuPos(id),
                           static_cast<float>(x),
                           static_cast<float>(y),
                           static_cast<float>(z));
        }

        // ── 5. IMU gyroscope — 3x 2-byte signed ──────────────────────────────
        else if (id == fl_imu_gyro_id || id == fr_imu_gyro_id ||
                 id == rl_imu_gyro_id || id == rr_imu_gyro_id) {

            int16_t x, y, z;
            memcpy(&x, &frame.data[0], 2);
            memcpy(&y, &frame.data[2], 2);
            memcpy(&z, &frame.data[4], 2);
            dm.setEcuGyro(getEcuPos(id),
                          static_cast<float>(x),
                          static_cast<float>(y),
                          static_cast<float>(z));
        }
    }

    close(s);
    printf("CAN Bridge: Stopped on %s\n", interface_name.c_str());
}

#endif // __linux__

// ─── Public API ───────────────────────────────────────────────────────────────

void startCanBridge() {
    if (running.load()) return;
    running.store(true);

#ifdef __linux__
    std::string target = "can0";

    struct ifreq ifr;
    int s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    strncpy(ifr.ifr_name, target.c_str(), IFNAMSIZ - 1);
    if (ioctl(s, SIOCGIFINDEX, &ifr) < 0) {
        printf("CAN Bridge: can0 not found, falling back to vcan0\n");
        target = "vcan0";
    }
    close(s);

    can_thread = std::thread(canReaderThread, target);
#else
    printf("CAN Bridge: SocketCAN not available on this platform\n");
#endif
}

void stopCanBridge() {
    if (!running.load()) return;
    running.store(false);
    if (can_thread.joinable())
        can_thread.join();
}

void injectCanFrame(int can_id, uint8_t* data, int len) {
    if (can_id >= 2048) return;
    uint64_t payload = 0;
    memcpy(&payload, data, (len > 8) ? 8 : len);
    observed_data[can_id] = payload;
    printf("CAN Bridge: Injected frame 0x%X (%d bytes)\n", can_id, len);
}