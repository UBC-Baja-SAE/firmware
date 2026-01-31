#include <iostream>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <unistd.h>
#include <thread>
#include <cstring>
#include "can_processor.h"
#include "can_id.h"

#define I2C_BUS "/dev/i2c-1"
#define IMU_ADDR 0x68

// Correct ICM-42670-P Registers from Datasheet
#define REG_PWR_MGMT0    0x11
#define REG_GYRO_DATA    0x19  // 6 bytes: X_H, X_L, Y_H, Y_L, Z_H, Z_L
#define REG_ACCEL_DATA   0x1F  // 6 bytes: X_H, X_L, Y_H, Y_L, Z_H, Z_L
#define REG_WHO_AM_I     0x75

int i2c_fd;
static bool i2c_running = false;

bool i2c_init() {
    i2c_fd = open(I2C_BUS, O_RDWR);
    if (i2c_fd < 0) return false;
    if (ioctl(i2c_fd, I2C_SLAVE, IMU_ADDR) < 0) return false;

    // 1. Verify Chip ID (Should be 0x67)
    uint8_t who_reg = REG_WHO_AM_I;
    uint8_t chip_id = 0;
    write(i2c_fd, &who_reg, 1);
    read(i2c_fd, &chip_id, 1);
    
    if (chip_id != 0x67) {
        printf("IMU ERROR: Wrong Chip ID 0x%02X (Expected 0x67)\n", chip_id);
        // return false;
    }

    // 2. Wake up: Set PWR_MGMT0 to 0x0F (Enable Accel & Gyro in Low Noise Mode)
    uint8_t setup[] = {REG_PWR_MGMT0, 0x0F};
    if (write(i2c_fd, setup, 2) != 2) return false;

    return true;
}

void poll_imu() {
    while (true) {
        uint8_t raw_gyro[6];
        uint8_t raw_accel[6];

        // Read Gyro
        uint8_t g_reg = REG_GYRO_DATA;
        write(i2c_fd, &g_reg, 1);
        if (read(i2c_fd, raw_gyro, 6) == 6) {
            uint64_t packed = 0;
            memcpy(&packed, raw_gyro, 6);
            observed_data[pi_imu_gyro_id] = packed;
        }

        // Read Accel
        uint8_t a_reg = REG_ACCEL_DATA;
        write(i2c_fd, &a_reg, 1);
        if (read(i2c_fd, raw_accel, 6) == 6) {
            uint64_t packed = 0;
            memcpy(&packed, raw_accel, 6);
            observed_data[pi_imu_accel_id] = packed;
        }
        
        // 100Hz Refresh Rate
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); 
    }
}

void start_i2c_processor() {
    if (i2c_running) return;

    if (i2c_init()) {
        i2c_running = true;
        std::thread i2c_thread(poll_imu);
        i2c_thread.detach(); 
        printf("I2C IMU Processor Started Successfully\n");
    } else {
        printf("Failed to initialize I2C IMU\n");
    }
}