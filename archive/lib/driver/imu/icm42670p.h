/**
 * @file icm42670p.h
 * @brief Driver for the ICM-42670-P IMU sensor.
 * 
 * This driver provides functionality to initialize the ICM-42670-P IMU sensor,
 * read sensor data, and configure the sensor settings.
 * 
 * The entry point for this module is the `ICM42670P` class, which handles
 * communication with the sensor over I2C. The class constructor initializes
 * the sensor and sets it up for operation, while the destructor cleans up
 * the file descriptor used for I2C communication. The class also provides
 * methods to write to and read from specific registers of the sensor, which
 * enables reading sensor data such as temperature, accelerometer, and
 * gyroscope values.
 * 
 * This driver is designed to be used in a Linux environment with I2C support.
 * @see https://invensense.tdk.com/download-pdf/icm-42670-p-datasheet/
 */

#ifndef ICM42670P_H
#define ICM42670P_H

#include "vector.h"
#include <cstdint>
#include <cstring>
#include <optional>

/**
 * @class ICM42670P
 * @brief The ICM-42670-P IMU sensor.
 */
class ICM42670P
{
private:
    /**
     * File descriptor for the IMU.
     */
    int file;

    /**
     * I2C slave address for the IMU.
     */
    const int address;

    /**
     * Writes a byte to a register.
     *
     * @param reg Register address to write to.
     * @param data Data byte to write to the register.
     * @return True if the write operation was successful, false otherwise.
     */
    bool write_to_register(uint8_t reg, uint8_t data);

    /**
     * Reads bytes from a register.
     * 
     * @param reg Register address of the start of the bytes to be read.
     * @param data Pointer to a buffer to contain the read bytes. If the read
     * operation is successful, the buffer will contain the data read
     * from the register addresses `reg` up to (but not including)
     * `reg + length`.
     * @param length Number of bytes to read.
     * @return True if the read operation was successful, false otherwise.
     */
    bool read_from_register(uint8_t reg, uint8_t* data, size_t length);

public:
    /**
     * The constructor for the ICM42670P class.
     * 
     * This constructor initializes the ICM-42670-P IMU sensor by opening
     * the I2C device interface, setting the I2C slave address, and writing the
     * power management register to wake up the device.
     * 
     * Initialises the gyroscope and accelerometer in their respective low noise
     * modes (on), and sets the accelerometer/gyroscope output data rates and
     * data scales.
     * 
     * Note that the constructor will sleep for 200 microseconds between setting
     * the power management register and any subsequent register writes, as
     * mandated by the ICM-42670-P datasheet.
     * 
     * @param device_path The file path to the I2C device interface utilised by
     * the IMU.
     * @param alt_address Whether the I2C slave address for this device is the
     * alternative address. If true, the slave address will be `0x69`, otherwise
     * it will be set to `0x68`. Used to connect multiple ICM-42670-P devices on
     * the same I2C bus. Default `false`.
     */
    ICM42670P(const char* device_path, bool alt_address = false);

    /**
     * Measures the acceleration observed by the IMU's accelerometer.
     * 
     * @return The 3-dimensional acceleration vector, where each element of the
     * vector is the acceleration (in standard gravity (g)) observed along its
     * respective axis.
     */
    std::optional<IMUVector> measureAcceleration();

    /**
     * Measures the angular velocity observed by the IMU's gyroscope.
     * 
     * @return The 3-dimensional angular velocity vector, where each element of
     * the vector is the angular velocity (in degrees per second (º/s)) observed
     * along its respective axis.
     */
    std::optional<IMUVector> measureAngularVelocity();

    /**
     * Measures the temperature of the IMU sensor.
     * 
     * @return The temperature of the IMU chip in degrees Celsius.
     */
    std::optional<double> measureTemperature();

    /**
     * Destructor for the ICM42670P class.
     * 
     * Closes the I2C device interface.
     */
    ~ICM42670P();
};

#endif /* ICM42670_H */