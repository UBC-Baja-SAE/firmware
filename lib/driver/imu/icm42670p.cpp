#include "icm42670p.h"
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <iomanip>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <chrono>
#include <thread>

/**
 * Power configuration register address. Defines the IMU sensor modes.
 */
#define PWR_MGMT0 0x1F

/**
 * Gyroscope configuration register address. Bits 6:5 define the scale for the
 * angular velocity, while bits 3:0 define the output data rate.
 */
#define GYRO_CONFIG0 0x20

/**
 * Accelerometer configuration register address. Bits 6:5 define the scale for
 * the acceleration, while bits 3:0 define the output data rate.
 */
#define ACCEL_CONFIG0 0x21

/**
 * Temperature data register base address. The temperature data is observed as
 * a 16-bit scalar value represented in big-endian.
 * 
 * The temperature bits `x` can be converted to degrees Celsius as follows:
 * `degrees Celsius = (x / 128) + 15`.
 */
#define REG_TEMP 0x09

/**
 * Accelerometer data register base address. The acceleration data is observed
 * as 3-dimensional vectors respresented in big-endian. Each dimension is stored
 * as 2 bytes (16 bits), where the upper bits (e.g. `x[15:8]`) are stored first,
 * in `0x0B`. The bits are stored in adjacent registers `0x0B`-`0x10`, denoting
 * x, y, and z respectively.
 * 
 * Each element is represented in bits/g, where the scaling factor is determined
 * by the scale configured at boot.
 */
#define REG_ACCEL 0x0B

/**
 * Gyroscope data register base address. The acceleration data is observed
 * as 3-dimensional vectors respresented in big-endian. Each dimension is stored
 * as 2 bytes (16 bits), where the upper bits (e.g. `x[15:8]`) are stored first,
 * in `0x0B`. The bits are stored in adjacent registers `0x0B`-`0x10`, denoting
 * x, y, and z respectively.
 * 
 * Each element is represented in bits/(degrees/seconds), where the scaling
 * factor is determined by the scale configured at boot.
 */
#define REG_GYRO 0x11

// Slave address for the ICM-42670-P IMU.
#define SLAVE_ADDR_BASE 0x68

bool ICM42670P::write_to_register(uint8_t reg, uint8_t data)
{
    // specify the register address, then the data we want to write
    uint8_t buf[2] = {reg, data};
    return write(file, buf, 2) == 2;
}

bool ICM42670P::read_from_register(uint8_t reg, uint8_t* data, size_t length)
{
    // open the register in write mode, then read our desired number of bytes
    if (write(file, &reg, 1) != 1)
    {
        return false;
    }
    return read(file, data, length) == length;
}

std::optional<IMUVector> ICM42670P::measureAcceleration()
{
    uint8_t accel_registers[6];

    if (!ICM42670P::read_from_register(REG_ACCEL, accel_registers, 6))
    {
        return std::nullopt;
    }

    uint16_t x = ((uint16_t) accel_registers[0] << 8) | accel_registers[1];
    uint16_t y = ((uint16_t) accel_registers[2] << 8) | accel_registers[3];
    uint16_t z = ((uint16_t) accel_registers[4] << 8) | accel_registers[5];

    IMUVector vector;
    double scale = 16384.0;
    // section 3.2, lsb -> lsb/(lsb/g) -> g, scale corresponds to boot config
    vector.x = x / scale;
    vector.y = y / scale;
    vector.z = z / scale;

    return vector;
}

std::optional<IMUVector> ICM42670P::measureAngularVelocity()
{
    uint8_t gyro_registers[6];

    if (!ICM42670P::read_from_register(REG_GYRO, gyro_registers, 6))
    {
        return std::nullopt;
    }

    uint16_t x = ((uint16_t) gyro_registers[0] << 8) | gyro_registers[1];
    uint16_t y = ((uint16_t) gyro_registers[2] << 8) | gyro_registers[3];
    uint16_t z = ((uint16_t) gyro_registers[4] << 8) | gyro_registers[5];

    IMUVector vector;
    double scale = 131.0;
    // section 3.1, lsb -> lsb/(lsb/(º/s)), scale corresponds to boot config
    vector.x = x / scale;
    vector.y = y / scale;
    vector.z = z / scale;

    return vector;
}

std::optional<double> ICM42670P::measureTemperature()
{
    uint8_t temp_registers[2];

    if (!ICM42670P::read_from_register(REG_TEMP, temp_registers, 2))
    {
        return std::nullopt;
    }

    uint16_t scalar = ((uint16_t) temp_registers[0] << 8) | temp_registers[1];

    return scalar / 128.0 + 25.0;
}

ICM42670P::ICM42670P(const char* device_path, bool alt_address)
    : address(SLAVE_ADDR_BASE ? alt_address : SLAVE_ADDR_BASE + 1)
{
    file = open(device_path, O_RDWR);
    if (file < 0)
    {
        throw std::runtime_error("Failed to open device file descriptor");
    }

    if (ioctl(file, I2C_SLAVE, address) < 0)
    {
        close(file);
        throw std::runtime_error("Failed to set I2C slave address");
    }

    // see `PWR_MGMT0` register in the datasheet
    // sets accel and gyro to "low noise" mode
    if (!ICM42670P::write_to_register(PWR_MGMT0, 0x0f))
    {
        close(file);
        throw std::runtime_error("Failed to write PWR_MGMT0");
    }

    // datasheet mandates 200 microsecond delay before next register write
    std::this_thread::sleep_for(std::chrono::microseconds(200));

    // set to 0bx11x1001 for ±250 dps scale and 100 Hz output data rate.
    if (!ICM42670P::write_to_register(GYRO_CONFIG0, 0x69))
    {
        throw std::runtime_error("Failed to set gyroscope configuration");
    }

    // set to 0bx11x1001 for ±2 g scale and 100 Hz output data rate.
    if (!ICM42670P::write_to_register(ACCEL_CONFIG0, 0x69))
    {
        throw std::runtime_error("Failed to set accelerometer configuration");
    }
}

ICM42670P::~ICM42670P()
{
    close(file);
}