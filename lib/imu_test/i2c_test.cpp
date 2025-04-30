#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <iomanip>
#include <sys/ioctl.h>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <thread>

// defined in least significant bits (LSB)
#define GYRO_SCALE ((2 << 15) / 250) // 131 LSB per degrees per second (DPS) 
#define ACCEL_SCALE (2 << 13) // 8192 LSB per standard gravity (g)

const char* I2C_DEVICE = "/dev/i2c-1";
const int ICM42670_ADDRESS = 0x68;

// Register addresses
const uint8_t PWR_MGMT0 = 0x1F;
const uint8_t GYRO_CONFIG0 = 0x20;
const uint8_t ACCEL_CONFIG0 = 0x21;

// Accel and Gyro data addresses
const uint8_t ACCEL_DATA_X1 = 0x0B;
const uint8_t ACCEL_DATA_X0 = 0x0C;
const uint8_t ACCEL_DATA_Y1 = 0x0D;
const uint8_t ACCEL_DATA_Y0 = 0x0E;
const uint8_t ACCEL_DATA_Z1 = 0x0F;
const uint8_t ACCEL_DATA_Z0 = 0x10;

const uint8_t GYRO_DATA_X1 = 0x11;
const uint8_t GYRO_DATA_X0 = 0x12;
const uint8_t GYRO_DATA_Y1 = 0x13;
const uint8_t GYRO_DATA_Y0 = 0x14;
const uint8_t GYRO_DATA_Z1 = 0x15;
const uint8_t GYRO_DATA_Z0 = 0x16;

// Temp data address
const uint8_t TEMP_DATA1 = 0x09;
const uint8_t TEMP_DATA0 = 0x0A;

bool writeByte(int file, uint8_t reg, uint8_t data) {
    uint8_t buf[2] = { reg, data };
    return (write(file, buf, 2) == 2);
}

bool readByte(int file, uint8_t reg, uint8_t& data) {
    if (write(file, &reg, 1) != 1) {
        return false;
    }
    if (read(file, &data, 1) != 1) {
        return false;
    }
    return true;
}

bool readSensorData(int file, uint8_t regHigh, uint8_t regLow, int16_t& outValue) {
    uint8_t high, low;
    if (!readByte(file, regHigh, high) || !readByte(file, regLow, low)) {
        return false;
    }
    outValue = (int16_t)((high << 8) | low);
    return true;
}

int main() {
    int file = open(I2C_DEVICE, O_RDWR);
    if (file < 0) {
        std::cerr << "Failed to open I2C device\n";
        return 1;
    }

    if (ioctl(file, I2C_SLAVE, ICM42670_ADDRESS) < 0) {
        std::cerr << "Failed to set I2C address\n";
        return 1;
    }

    // Wake up device
    if (!writeByte(file, PWR_MGMT0, 0x0F)) {
        std::cerr << "Failed to write PWR_MGMT0\n";
        return 1;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Configure Gyroscope: ±500 dps, 400 Hz
    if (!writeByte(file, GYRO_CONFIG0, 0x27)) {
        std::cerr << "Failed to write GYRO_CONFIG0\n";
        return 1;
    }

    // Configure Accelerometer: ±4g, 400 Hz
    if (!writeByte(file, ACCEL_CONFIG0, 0x27)) {
        std::cerr << "Failed to write ACCEL_CONFIG0\n";
        return 1;
    }

    std::cout << "ICM-42670-P Initialized.\n\n";

    while (true) {
        int16_t accel_x, accel_y, accel_z;
        int16_t gyro_x, gyro_y, gyro_z;
        int16_t temp_raw;

        // Read accelerometer
        if (!readSensorData(file, ACCEL_DATA_X1, ACCEL_DATA_X0, accel_x) ||
            !readSensorData(file, ACCEL_DATA_Y1, ACCEL_DATA_Y0, accel_y) ||
            !readSensorData(file, ACCEL_DATA_Z1, ACCEL_DATA_Z0, accel_z)) {
            std::cerr << "Failed to read accelerometer data\n";
            continue;
        }

        // Read gyroscope
        if (!readSensorData(file, GYRO_DATA_X1, GYRO_DATA_X0, gyro_x) ||
            !readSensorData(file, GYRO_DATA_Y1, GYRO_DATA_Y0, gyro_y) ||
            !readSensorData(file, GYRO_DATA_Z1, GYRO_DATA_Z0, gyro_z)) {
            std::cerr << "Failed to read gyroscope data\n";
            continue;
        }

        // Read temperature (only TEMP_DATA0)
        if (!readSensorData(file, TEMP_DATA1, TEMP_DATA0, temp_raw)) {
            std::cerr << "Failed to read temperature data\n";
            continue;
        }
        float temp_c = (temp_raw / 128.0f) + 25.0f;

        // Convert to g and dps
        float accel_scale = 4.0f / 32768.0f;   // ±4g
        float gyro_scale = 250.0f / 32768.0f;  // ±500 dps

        float ax = accel_x * accel_scale;
        float ay = accel_y * accel_scale;
        float az = accel_z * accel_scale;

        float gx = gyro_x * gyro_scale;
        float gy = gyro_y * gyro_scale;
        float gz = gyro_z * gyro_scale;

        // Print
		std::cout << std::fixed << std::setprecision(3); // Always 3 decimals

		std::cout << "Accel [g]:  X: " << std::setw(8) << ax
				  << "  Y: " << std::setw(8) << ay
				  << "  Z: " << std::setw(8) << az
				  << "   Gyro [dps]: X: " << std::setw(8) << gx
				  << "  Y: " << std::setw(8) << gy
				  << "  Z: " << std::setw(8) << gz
				  << "   Temp [C]: " << std::setw(8) << temp_c
				  << "\n";

        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // ~10 Hz
    }

    close(file);
    return 0;
}

