#include "MLX90614.h"

/**
 * @brief Initializes the MLX90614 sensor handle.
 */
void MLX90614_Init(MLX90614_Handle *dev, I2C_HandleTypeDef *hi2c) {
    dev->hi2c = hi2c;
}

/**
 * @brief Reads temperature from the sensor.
 */
HAL_StatusTypeDef MLX90614_Read_Temp(MLX90614_Handle *dev, uint8_t reg_addr, float *temp_data) {
    uint8_t i2c_buf[3];
    HAL_StatusTypeDef status;

    // Read 3 bytes of data from the sensor's RAM (2 data bytes + 1 PEC byte)
    status = HAL_I2C_Mem_Read(dev->hi2c, MLX90614_I2C_ADDR, reg_addr, I2C_MEMADD_SIZE_8BIT, i2c_buf, 3, HAL_MAX_DELAY);

    if (status == HAL_OK) {
        // The first two bytes are the temperature data (LSB, MSB)
        uint16_t raw_temp = (uint16_t)i2c_buf[0] | ((uint16_t)i2c_buf[1] << 8);

        // Check if data is valid (not zero)
        if (raw_temp == 0) {
            return HAL_ERROR; // Return an error if read data is zero
        }

        // Convert the raw data to temperature in Celsius
        // Formula: Temperature = (RAW_DATA * 0.02) - 273.15
        *temp_data = ((float)raw_temp * 0.02f) - 273.15f;
    }

    return status;
}
