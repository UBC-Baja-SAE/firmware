/**
 * @file driver.cpp
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

#include <linux/i2c-dev.h>
#include <iomanip>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <sys/ioctl.h>
#include <chrono>
#include <thread>

// configuration register addresses
#define PWR_MGMT0 0x1F
#define GYRO_CONFIG0 0x20
#define ACCEL_CONFIG0 0x21

// data register addresses; occupies 0x09 to 0x16
#define TEMP_DATA1 0x09
#define TEMP_DATA0 0x0A

#define ACCEL_DATA_X1 0x0B
#define ACCEL_DATA_X0 0x0C
#define ACCEL_DATA_Y1 0x0D
#define ACCEL_DATA_Y0 0x0E
#define ACCEL_DATA_Z1 0x0F
#define ACCEL_DATA_Z0 0x10

#define GYRO_DATA_X1 0x11
#define GYRO_DATA_X0 0x12
#define GYRO_DATA_Y1 0x13
#define GYRO_DATA_Y0 0x14
#define GYRO_DATA_Z1 0x15
#define GYRO_DATA_Z0 0x16

const char* I2C_DEVICE = "/dev/i2c-1";
const int IMU_ADDRESS = 0x68;

/**
 * @class ICM42670P
 * @brief The ICM-42670-P IMU sensor.
 */
class ICM42670P
{
private:
    /**
     * File descriptor for the I2C device.
     */
    int file;

    /**
     * Writes a byte to a specific register of the IMU.
     * 
     * @param fd File descriptor for the ICM-42670-P device.
     * @param reg Register address to write to.
     * @param data Data byte to write to the register.
     * @return True if the write operation was successful, false otherwise.
     */
    bool write_to_register(int fd, uint8_t reg, uint8_t data)
    {
        // need to specify the register address, then the data we want to write
        uint8_t buf[2] = { reg, data };
        return write(fd, buf, 2) == 2;
    }

    /**
     * Reads a given number of bytes from a given register of the IMU.
     * 
     * @param fd File descriptor for the ICM-42670-P device.
     * @param reg Register address to start reading from.
     * @param data Pointer to a buffer where the read data will be stored. If
     * the read operation is successful, the buffer will contain the data read
     * from the register addresses `reg` up to (but not including)
     * `reg + length`.
     * @param length Number of bytes to read.
     * @return True if the read operation was successful, false otherwise.
     */
    bool read_from_register(int fd, uint8_t reg, uint8_t* data, size_t length)
    {
        // to read from a register, we need to open the register in write mode,
        // then read our desired number of bytes
        if (write(fd, &reg, 1) != 1)
        {
            return false;
        }
        return read(fd, data, length) == length;
    }

public:
    /**
     * The constructor for the ICM42670P class.
     * 
     * This constructor initializes the ICM-42670-P IMU sensor by opening
     * the I2C device file, setting the I2C slave address, and writing the
     * power management register to wake up the device.
     * 
     * Note that the constructor will sleep for 200 microseconds between setting
     * the power management register and any subsequent register writes, as
     * mandated by the ICM-42670-P datasheet.
     * 
     * @param device_path The file path to the I2C device file that the device
     * is connected to, default is `"/dev/i2c-1"`.
     * @param slave_address_high If true, the I2C slave address for this device
     * will be set to `0x69`, otherwise it will be set to `0x68`. This is
     * useful for connecting multiple ICM-42670-P devices on the same I2C bus.
     * Default is `false`.
     */
    ICM42670P(
        const char* device_path = I2C_DEVICE,
        bool slave_address_high = false
    ) {
        file = open(I2C_DEVICE, O_RDWR);
        if (file < 0)
        {
            throw std::runtime_error("Failed to open device file descriptor");
        }

        // the ICM-42670-P can be configured to use either 0x68 or 0x69 as its
        // I2C address, depending on the `AP_AD0` pin, see the datasheet
        if (ioctl(file, I2C_SLAVE, IMU_ADDRESS | slave_address_high) < 0)
        {
            close(file);
            throw std::runtime_error("Failed to set I2C slave address");
        }

        // see `PWR_MGMT0` register in the datasheet
        // sets accel and gyro to "low noise" mode
        if (!write_to_register(file, PWR_MGMT0, 0x0f))
        {
            close(file);
            throw std::runtime_error("Failed to write PWR_MGMT0");
        }

        // datasheet mandates 200 microsecond delay before next register write
        std::this_thread::sleep_for(std::chrono::microseconds(200));
    }

    ~ICM42670P()
    {
        close(file);
    }
}