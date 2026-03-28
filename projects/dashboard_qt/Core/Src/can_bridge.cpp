#include "../Inc/can_bridge.h"
#include "../Inc/can_id.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <thread>
#include <atomic>

#ifdef __linux__
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/socket.h>
#include <net/if.h>
#include <unistd.h>
#include <sys/ioctl.h>
#endif

// Global data array
volatile uint64_t observed_data[2048] = {0};

// Thread control
static std::atomic<bool> running(false);
static std::thread can_thread;

/**
 * @brief Observes the last received CAN message with the given id.
 * @param id    the id of the CAN message to observe.
 * @return the data received from the CAN message.
 */
double getData(int id) {
    uint64_t data = observed_data[id];

    if (data != 0) {
        printf("DEBUG: ID 0x%X received 0x%016llx\n", id, (unsigned long long)data);
    }

    // -- 6 byte IMU data --
    if (id == pi_imu_accel_id || id == pi_imu_gyro_id ||
        id == fl_imu_accel_id || id == fl_imu_gyro_id ||
        id == fr_imu_accel_id || id == fr_imu_gyro_id ||
        id == rl_imu_accel_id || id == rl_imu_gyro_id ||
        id == rr_imu_accel_id || id == rr_imu_gyro_id) {

        double raw_bits;
        memset(&raw_bits, 0, sizeof(double));
        memcpy(&raw_bits, (const void*)&observed_data[id], 6);

        return raw_bits;
    }

    // --- 2-Byte Sensors (Suspension / Strain) ---
    if (id >= 0x100 && id <= 0x134) {
        uint16_t val;
        memcpy(&val, &data, 2);
        return (double)val;
    }

    // --- 4-Byte Sensors (Speedometer 0x201, Tachometer 0x200) ---
    if (id == 0x200 || id == 0x201) {
        uint32_t val;
        memcpy(&val, &data, 4);
        return (double)val;
    }

    // --- 8-Byte Sensors (Everything else) ---
    double result;
    memcpy(&result, &data, sizeof(data));
    return result;
}

#ifdef __linux__
static void canReaderThread() {
    int s;
    struct sockaddr_can addr;
    struct ifreq ifr;

    if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
        perror("CAN Bridge: Socket Error");
        return;
    }

    strcpy(ifr.ifr_name, "can0");
    if (ioctl(s, SIOCGIFINDEX, &ifr) < 0) {
        perror("CAN Bridge: Interface Error");
        close(s);
        return;
    }

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("CAN Bridge: Bind Error");
        close(s);
        return;
    }

    struct can_frame frame;
    printf("CAN Bridge: Listening on can0...\n");

    while (running.load()) {
        int nbytes = read(s, &frame, sizeof(struct can_frame));

        if (nbytes > 0) {
            int id = frame.can_id & 0x7FF; // Mask to 11-bit ID

            if (id < 2048) {
                uint64_t payload = 0;
                memcpy(&payload, frame.data, frame.can_dlc);
                observed_data[id] = payload;
            }
        }
    }

    close(s);
    printf("CAN Bridge: Stopped\n");
}
#else
static void canReaderThread() {
    printf("CAN Bridge: SocketCAN not available on this platform\n");
    // TODO: Implement UART or other interface for non-Linux platforms
}
#endif

void startCanBridge() {
    if (!running.load()) {
        running.store(true);
        can_thread = std::thread(canReaderThread);
        printf("CAN Bridge: Started\n");
    }
}

void stopCanBridge() {
    if (running.load()) {
        running.store(false);
        if (can_thread.joinable()) {
            can_thread.join();
        }
    }
}

void injectCanFrame(int can_id, uint8_t* data, int len) {
    if (can_id < 2048) {
        uint64_t payload = 0;
        memcpy(&payload, data, len);
        observed_data[can_id] = payload;
        printf("CAN Bridge: Injected frame 0x%X with %d bytes\n", can_id, len);
    }
}