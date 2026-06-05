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

// ─── Suspension Command CAN ID ────────────────────────────────────────────────
//
// Must match the ID checked in the ECU's HAL_FDCAN_RxFifo0Callback (0x300).
// Byte 0: cmd       — 0x01 = go to setting, 0x11 = calibrate/home
// Byte 1: setting   — 0=Soft, 1=Profile1, 2=Profile2, 3=Hard (ignored for 0x11)
//
#define SUSPENSION_MODE_CMD_ID 0x300

#ifdef __linux__

static int can_socket = -1; // shared so sendCanCommand() can write without a second socket

// ─── Helpers ──────────────────────────────────────────────────────────────────

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

    return DataManager::FRONT_LEFT;
}

// ─── CAN Reader Thread ────────────────────────────────────────────────────────

static void canReaderThread(std::string interface_name) {
    struct sockaddr_can addr;
    struct ifreq ifr;

    if ((can_socket = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
        perror("CAN Bridge: Socket Error");
        return;
    }

    strncpy(ifr.ifr_name, interface_name.c_str(), IFNAMSIZ - 1);
    if (ioctl(can_socket, SIOCGIFINDEX, &ifr) < 0) {
        fprintf(stderr, "CAN Bridge: Interface '%s' not found\n", interface_name.c_str());
        close(can_socket);
        can_socket = -1;
        return;
    }

    addr.can_family  = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(can_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("CAN Bridge: Bind Error");
        close(can_socket);
        can_socket = -1;
        return;
    }

    printf("CAN Bridge: Listening on %s...\n", interface_name.c_str());

    auto& dm = DataManager::getInstance();
    struct can_frame frame;

    while (running.load()) {
        int nbytes = read(can_socket, &frame, sizeof(struct can_frame));
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

            if      (id == tachometer_id)  dm.setTach(val * 4);
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

        // ── 3. Strain gauges — 2-byte signed ─────────────────────────────────
        else if (id == fl_strain_l_id || id == fr_strain_l_id ||
                 id == rl_strain_l_id || id == rr_strain_l_id) {

            int16_t val;
            memcpy(&val, frame.data, 2);
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

    close(can_socket);
    can_socket = -1;
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

// ─── Suspension Mode Command ───────────────────────────────────────────────────
//
// Sends a 2-byte CAN frame to the suspension ECU.
// The ECU ISR reads the flags and the main loop calls the motor functions.
//
// Foxglove Publish panel JSON commands (topic: /can/command, encoding: json):
//   { "command": "SET_MODE_PROFILE0" }      → setting 0: Full Soft / Home
//   { "command": "SET_MODE_PROFILE1" }  → setting 1: N17=450,  N23=600
//   { "command": "SET_MODE_PROFILE2" }  → setting 2: N17=1200, N23=300
//   { "command": "SET_MODE_PROFILE3" }      → setting 3: Full Hard
//   { "command": "CALIBRATE" }          → homes both motors to position 0
//
void sendCanCommand(const std::string& command) {
#ifdef __linux__
    if (can_socket < 0) {
        fprintf(stderr, "CAN Bridge: Cannot send command — socket not open\n");
        return;
    }

    struct can_frame frame = {};
    frame.can_id  = SUSPENSION_MODE_CMD_ID;
    frame.can_dlc = 2;

    if (command == "SET_MODE_PROFILE0") {
        frame.data[0] = 0x01;
        frame.data[1] = 0;
    } else if (command == "SET_MODE_PROFILE1") {
        frame.data[0] = 0x01;
        frame.data[1] = 1;
    } else if (command == "SET_MODE_PROFILE2") {
        frame.data[0] = 0x01;
        frame.data[1] = 2;
    } else if (command == "SET_MODE_PROFILE3") {
        frame.data[0] = 0x01;
        frame.data[1] = 3;
    } else if (command == "CALIBRATE") {
        frame.data[0] = 0x11;  // Emergency reset / home
        frame.data[1] = 0x00;  // unused
    } else {
        fprintf(stderr, "CAN Bridge: Unknown suspension command: %s\n", command.c_str());
        return;
    }

    if (write(can_socket, &frame, sizeof(struct can_frame)) < 0) {
        perror("CAN Bridge: Failed to send suspension command");
    } else {
        printf("CAN Bridge: Sent '%s' → CAN ID 0x%03X [%02X %02X]\n",
               command.c_str(), SUSPENSION_MODE_CMD_ID,
               frame.data[0], frame.data[1]);
    }
#else
    (void)command;
    printf("CAN Bridge: SocketCAN not available, cannot send command\n");
#endif
}