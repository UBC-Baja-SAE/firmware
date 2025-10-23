#ifndef INC_MLX90614_H_
#define INC_MLX90614_H_

#include "stm32f1xx_hal.h" // Ensure this matches your specific STM32 series

// MLX90614 Default I2C Address
#define MLX90614_I2C_ADDR (0x5A << 1) // Default slave address shifted left for HAL

// MLX90614 RAM Register Addresses
#define MLX90614_AMBIENT_TEMP_RAM 	0x06  // Ambient Temperature
#define MLX90614_OBJECT_TEMP_RAM 	0x07  // Object Temperature

/**
 * @brief MLX90614 Sensor Handle Structure
 */
typedef struct {
    I2C_HandleTypeDef *hi2c; // Pointer to the I2C handle
} MLX90614_Handle;

/**
 * @brief Initializes the MLX90614 sensor handle.
 * @param dev Pointer to the MLX90614 handle structure.
 * @param hi2c Pointer to the HAL I2C handle.
 */
void MLX90614_Init(MLX90614_Handle *dev, I2C_HandleTypeDef *hi2c);

/**
 * @brief Reads a temperature value from the sensor.
 * @param dev Pointer to the MLX90614 handle structure.
 * @param reg_addr The register address to read from (ambient or object).
 * @param temp_data Pointer to a float where the temperature in Celsius will be stored.
 * @return HAL_StatusTypeDef HAL status of the I2C communication.
 */
HAL_StatusTypeDef MLX90614_Read_Temp(MLX90614_Handle *dev, uint8_t reg_addr, float *temp_data);


#endif /* INC_MLX90614_H_ */
