#include "control.h"
#include <string.h>

// Hardware Handles
extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;
extern FDCAN_HandleTypeDef hfdcan1;
extern I2C_HandleTypeDef hi2c1;

// Global state
static uint8_t g_imu_initialized = 0;
uint8_t pot_tx_data[2];

<<<<<<< Updated upstream
// Global variables for CLion Live Watches
volatile float live_gyro_x = 0.0f;
volatile float live_gyro_y = 0.0f;
volatile float live_gyro_z = 0.0f;

volatile float live_accel_x = 0.0f;
volatile float live_accel_y = 0.0f;
volatile float live_accel_z = 0.0f;

volatile float live_pot = 0.0f;

// Sensitivity for ICM-42670-P at default ±2000 dps is 16.4 LSB/dps
#define GYRO_SENSITIVITY_2000DPS 16.4f
#define ACCEL_SENSITIVITY_16G 2048.0f




    static void CAN_Transmit(uint32_t id, uint8_t *data, uint32_t len) {
        FDCAN_TxHeaderTypeDef tx_header = {
            .Identifier = id,
            .IdType             = FDCAN_STANDARD_ID,
            .TxFrameType        = FDCAN_DATA_FRAME,
            .DataLength         = len,
            .ErrorStateIndicator= FDCAN_ESI_ACTIVE,
            .BitRateSwitch      = FDCAN_BRS_OFF,
            .FDFormat           = FDCAN_CLASSIC_CAN,
            .TxEventFifoControl = FDCAN_NO_TX_EVENTS,
            .MessageMarker      = 0
        };

        // CHANGE: Do not call Error_Handler() if the queue is full. Just skip.
        if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &tx_header, data) != HAL_OK) {
            //Error_Handler();
            return;
        }
}

// Initialization for ICM-42670-P
void IMU_Init(void) {
    // Wake up: Enable both Accel and Gyro in Low Noise mode
    uint8_t pwr_mgmt = 0x0F;
    HAL_I2C_Mem_Write(&hi2c1, ICM_ADDR, PWR_MGMT0, I2C_MEMADD_SIZE_8BIT, &pwr_mgmt, 1, 1000);
=======
// Debug variables for Live Expression Watch
uint8_t g_debug_gyro_can_data[6];
volatile HAL_StatusTypeDef g_debug_i2c_status = HAL_OK;
volatile uint32_t g_debug_can_tx_errors = 0;
volatile uint32_t g_debug_i2c_recoveries = 0;

/**
 * @brief Non-fatal CAN transmit - logs error but continues operation
 * @param id CAN message ID
 * @param data Pointer to data buffer
 * @param len Data length (FDCAN_DLC_BYTES_x)
 * @return HAL_OK on success, HAL_ERROR on failure
 */
static HAL_StatusTypeDef CAN_Transmit(uint32_t id, uint8_t *data,
                                      uint32_t len) {
  FDCAN_TxHeaderTypeDef tx_header = {.Identifier = id,
                                     .IdType = FDCAN_STANDARD_ID,
                                     .TxFrameType = FDCAN_DATA_FRAME,
                                     .DataLength = len,
                                     .ErrorStateIndicator = FDCAN_ESI_ACTIVE,
                                     .BitRateSwitch = FDCAN_BRS_OFF,
                                     .FDFormat = FDCAN_CLASSIC_CAN,
                                     .TxEventFifoControl = FDCAN_NO_TX_EVENTS,
                                     .MessageMarker = 0};

  if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &tx_header, data) != HAL_OK) {
    // Non-fatal: increment error counter and continue
    g_debug_can_tx_errors++;
    return HAL_ERROR;
  }
  return HAL_OK;
}

/**
 * @brief I2C Bus Recovery - generates clock pulses to release stuck bus
 *
 * When an I2C transaction is interrupted (e.g., during debug or reset),
 * the slave may hold SDA low waiting for more clocks. This function
 * generates 9 clock pulses on SCL to allow the slave to release SDA.
 *
 * Hardware pins: PB6 = I2C1_SCL, PB7 = I2C1_SDA
 *
 * @return HAL_OK on success, HAL_ERROR if bus still stuck
 */
HAL_StatusTypeDef I2C_BusRecovery(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  // Force reset the I2C peripheral via RCC
  __HAL_RCC_I2C1_FORCE_RESET();
  HAL_Delay(2);
  __HAL_RCC_I2C1_RELEASE_RESET();
  HAL_Delay(2);

  // Disable I2C peripheral
  HAL_I2C_DeInit(&hi2c1);

  // Configure I2C1 pins: SCL (PB6), SDA (PB7)
  __HAL_RCC_GPIOB_CLK_ENABLE();

  // SCL (PB6) as open-drain output
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  // SDA (PB7) as input with pull-up to check state
  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  // Set SCL high initially
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
  HAL_Delay(1);

  // Generate 9 clock pulses to release stuck slave
  for (int i = 0; i < I2C_RECOVERY_MAX_CLOCKS; i++) {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET); // SCL low
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET); // SCL high
    HAL_Delay(1);

    // Check if SDA is released (high)
    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7) == GPIO_PIN_SET) {
      break;
    }
  }

  // Generate STOP condition: SDA low while SCL high, then SDA high
  // First set SDA (PB7) as output
  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET); // SDA low
  HAL_Delay(1);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET); // SCL high
  HAL_Delay(1);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET); // SDA high (STOP)
  HAL_Delay(1);

  // Re-configure pins for I2C alternate function
  // According to HAL MSP: PB6 and PB7 use GPIO_AF4_I2C1
  GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL; // Match original MSP config
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  // Re-enable I2C peripheral clock (in case reset disabled it)
  __HAL_RCC_I2C1_CLK_ENABLE();

  // Re-initialize I2C peripheral with original settings
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00707CBB;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

  if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
    return HAL_ERROR;
  }

  HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE);
  HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0);

  g_debug_i2c_recoveries++;
  return HAL_OK;
}

/**
 * @brief Check if IMU is responding (WHO_AM_I check)
 * @return 1 if responding correctly, 0 otherwise
 */
uint8_t IMU_IsResponding(void) {
  uint8_t who_am_i = 0;

  if (HAL_I2C_Mem_Read(&hi2c1, ICM_ADDR, WHO_AM_I, I2C_MEMADD_SIZE_8BIT,
                       &who_am_i, 1, 50) == HAL_OK) {
    return (who_am_i == WHO_AM_I_VAL) ? 1 : 0;
  }
  return 0;
>>>>>>> Stashed changes
}

/**
 * @brief Initialize ICM-42670-P IMU with retry and recovery
 * @return HAL_OK on success, HAL_ERROR if IMU not responding after retries
 */
HAL_StatusTypeDef IMU_Init(void) {
  uint8_t check = 0;
  uint8_t data = 0;
  HAL_StatusTypeDef status;

<<<<<<< Updated upstream


void SendPotOnCan(uint32_t can_id)
    {
        const int samples = 10;
        float posSum = 0;

        // DELETE THESE LINES (The ADC is already configured in main.c!)
        ADC_ChannelConfTypeDef sConfig = {0};
     //         Reconfigure ADC1 to the Potentiometer channel (PB1)
     sConfig.Channel = ADC_CHANNEL_5;
     sConfig.Rank = ADC_REGULAR_RANK_1;
     sConfig.SamplingTime = ADC_SAMPLETIME_64CYCLES_5; // Decreased
     sConfig.SingleDiff = ADC_SINGLE_ENDED;
     HAL_ADC_ConfigChannel(&hadc1, &sConfig);

        for (int i = 0; i < samples; i++)
        {
            HAL_ADC_Start(&hadc1);
            if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
            {
                uint32_t adcRaw = HAL_ADC_GetValue(&hadc1);
                float voltage = ((float)adcRaw * 1.8f) / 65535.0f; // Note: H7 is usually 3.3V
                float position = VoltageToPosition(voltage);
                posSum += position;
            }
            HAL_ADC_Stop(&hadc1);
        }

        float posAvg = posSum / samples;

        uint16_t posCan = (uint16_t)(posAvg * 100.0f);
        live_pot = posAvg;
        pot_tx_data[0] = (posCan >> 8) & 0xFF;
        pot_tx_data[1] = posCan & 0xFF;

        CAN_Transmit(can_id, pot_tx_data, FDCAN_DLC_BYTES_2);
    }



// void SendPotOnCan(uint32_t can_id)
// {
//     const int samples = 10;
//     float posSum = 0;
//     ADC_ChannelConfTypeDef sConfig = {0};
//
//     // // Reconfigure ADC1 to the Potentiometer channel (PB1)
//     // sConfig.Channel = ADC_CHANNEL_5;
//     // sConfig.Rank = ADC_REGULAR_RANK_1;
//     // sConfig.SamplingTime = ADC_SAMPLETIME_64CYCLES_5; // Decreased
//     // sConfig.SingleDiff = ADC_SINGLE_ENDED;
//     // HAL_ADC_ConfigChannel(&hadc1, &sConfig);
//
//     for (int i = 0; i < samples; i++)
//     {
//         HAL_ADC_Start(&hadc1);
//         if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
//         {
//             uint32_t adcRaw = HAL_ADC_GetValue(&hadc1);
//             float voltage = ((float)adcRaw * 3.3f) / 65535.0f; // Note: H7 is usually 3.3V
//             float position = VoltageToPosition(voltage);
//             posSum += position;
//         }
//         else
//         {
//             // Add this breakpoint or variable!
//             live_pot = -1.0f; // If you see -1.0 in Live Watch, you know the ADC is timing out!
//         }
//         HAL_ADC_Stop(&hadc1);
//     }
//         float posAvg = posSum / samples;
//
//         uint16_t posCan = (uint16_t)(posAvg * 100.0f);
//         live_pot = (posAvg *100.0f);
//         pot_tx_data[0] = (posCan >> 8) & 0xFF;
//         pot_tx_data[1] = posCan & 0xFF;
//
//         CAN_Transmit(can_id, pot_tx_data, FDCAN_DLC_BYTES_2);
//
// }

void SendAccelOnCan(uint32_t can_id) {
    uint8_t raw_data[6];

    if (HAL_I2C_Mem_Read(&hi2c1, ICM_ADDR, ACCEL_DATA_START, I2C_MEMADD_SIZE_8BIT, raw_data, 6, 50) == HAL_OK) {

        // 1. Combine high and low bytes into signed 16-bit integers
        int16_t raw_x = (int16_t)((raw_data[0] << 8) | raw_data[1]);
        int16_t raw_y = (int16_t)((raw_data[2] << 8) | raw_data[3]);
        int16_t raw_z = (int16_t)((raw_data[4] << 8) | raw_data[5]);

        // 2. Convert to Gs (or m/s^2 by multiplying by 9.81)
        live_accel_x = (float)raw_x / ACCEL_SENSITIVITY_16G;
        live_accel_y = (float)raw_y / ACCEL_SENSITIVITY_16G;
        live_accel_z = (float)raw_z / ACCEL_SENSITIVITY_16G;

        // 3. Keep your existing CAN transmission
        CAN_Transmit(can_id, raw_data, FDCAN_DLC_BYTES_6);
    }
        else {
            HAL_I2C_DeInit(&hi2c1);
            HAL_I2C_Init(&hi2c1);
            return;
        }

}
/*void SendGyroOnCan(uint32_t can_id) {
    uint8_t raw_data[6];
    // Read 6 bytes starting from GYRO_DATA_X1 (0x11)
    if (HAL_I2C_Mem_Read(&hi2c1, ICM_ADDR, GYRO_DATA_START, I2C_MEMADD_SIZE_8BIT, raw_data, 6, 100) == HAL_OK) {
        CAN_Transmit(can_id, raw_data, FDCAN_DLC_BYTES_6);
    }
}*/

void SendGyroOnCan(uint32_t can_id) {
    uint8_t raw_data[6];

    // Read 6 bytes starting from GYRO_DATA_X1 (0x11)
    if (HAL_I2C_Mem_Read(&hi2c1, ICM_ADDR, GYRO_DATA_START, I2C_MEMADD_SIZE_8BIT, raw_data, 6, 100) == HAL_OK) {

        // Convert raw bytes to signed 16-bit integers
        int16_t raw_x = (int16_t)((raw_data[0] << 8) | raw_data[1]);
        int16_t raw_y = (int16_t)((raw_data[2] << 8) | raw_data[3]);
        int16_t raw_z = (int16_t)((raw_data[4] << 8) | raw_data[5]);

        // Convert to Degrees Per Second (DPS)
        live_gyro_x = (float)raw_x / GYRO_SENSITIVITY_2000DPS;
        live_gyro_y = (float)raw_y / GYRO_SENSITIVITY_2000DPS;
        live_gyro_z = (float)raw_z / GYRO_SENSITIVITY_2000DPS;

        CAN_Transmit(can_id, raw_data, FDCAN_DLC_BYTES_6);

    }
    else {
        HAL_I2C_DeInit(&hi2c1);
        HAL_I2C_Init(&hi2c1);
        return;
    }
=======
  g_imu_initialized = 0;

  // Try initialization with retries
  for (int retry = 0; retry < IMU_INIT_RETRY_COUNT; retry++) {
    // Check WHO_AM_I with short timeout
    status = HAL_I2C_Mem_Read(&hi2c1, ICM_ADDR, WHO_AM_I, I2C_MEMADD_SIZE_8BIT,
                              &check, 1, 50);

    if (status == HAL_OK && check == WHO_AM_I_VAL) {
      // Configure Gyroscope: +/- 2000 dps, ODR = 100 Hz
      // FS (6:5): 00 = 2000dps, ODR (3:0): 0111 = 100Hz
      data = 0x07;
      status = HAL_I2C_Mem_Write(&hi2c1, ICM_ADDR, GYRO_CONFIG0,
                                 I2C_MEMADD_SIZE_8BIT, &data, 1, 50);
      if (status != HAL_OK)
        continue;

      // Power Management: Gyro LN Mode, Accel Off
      // GYRO_MODE (3:2): 11 = Low Noise, ACCEL_MODE (1:0): 00 = Off
      data = 0x0C;
      status = HAL_I2C_Mem_Write(&hi2c1, ICM_ADDR, PWR_MGMT0,
                                 I2C_MEMADD_SIZE_8BIT, &data, 1, 50);
      if (status != HAL_OK)
        continue;

      // Wait for gyro to stabilize after power mode change
      HAL_Delay(IMU_STARTUP_DELAY_MS);

      g_imu_initialized = 1;
      return HAL_OK;
    }

    // I2C communication failed, try bus recovery
    I2C_BusRecovery();
    HAL_Delay(50);
  }

  return HAL_ERROR;
}

float VoltageToPosition(float voltage) {
  return X_EXTEND_CM +
         (voltage * (X_EXTEND_CM - X_COMP_CM) / (V_EXTEND - V_COMP));
}

void SendPotOnCan(uint32_t can_id) {
  const int samples = 10;
  float posSum = 0;
  ADC_ChannelConfTypeDef sConfig = {0};

  // Configure ADC1 to the Potentiometer channel
  sConfig.Channel = ADC_CHANNEL_5;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_64CYCLES_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  HAL_ADC_ConfigChannel(&hadc1, &sConfig);

  for (int i = 0; i < samples; i++) {
    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
      uint32_t adcRaw = HAL_ADC_GetValue(&hadc1);
      float voltage = ((float)adcRaw * 3.3f) / 65535.0f;
      float position = VoltageToPosition(voltage);
      posSum += position;
    }
    HAL_ADC_Stop(&hadc1);
  }

  float posAvg = posSum / samples;
  uint16_t posCan = (uint16_t)(posAvg * 100.0f);
  pot_tx_data[0] = (posCan >> 8) & 0xFF;
  pot_tx_data[1] = posCan & 0xFF;

  CAN_Transmit(can_id, pot_tx_data, FDCAN_DLC_BYTES_2);
}

/**
 * @brief Read gyroscope data and transmit on CAN with robust error recovery
 * @param can_id CAN message ID for gyro data
 */
void SendGyroOnCan(uint32_t can_id) {
  uint8_t raw_data[6] = {0};

  // Skip if IMU was never initialized
  if (!g_imu_initialized) {
    return;
  }

  // Check if I2C is stuck in BUSY state from previous failed transfer
  if (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY) {
    // Abort any ongoing transfer
    HAL_I2C_Master_Abort_IT(&hi2c1, ICM_ADDR);
    HAL_Delay(5);

    // If still not ready, do full bus recovery
    if (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY) {
      I2C_BusRecovery();
      IMU_Init();
      return; // Skip this cycle
    }
  }

  // Read 6 bytes starting from GYRO_DATA_X1 (0x11) with short timeout
  g_debug_i2c_status = HAL_I2C_Mem_Read(&hi2c1, ICM_ADDR, GYRO_DATA_START,
                                        I2C_MEMADD_SIZE_8BIT, raw_data, 6, 50);

  if (g_debug_i2c_status == HAL_OK) {
    // Success: copy debug data and transmit
    memcpy(g_debug_gyro_can_data, raw_data, 6);
    CAN_Transmit(can_id, raw_data, FDCAN_DLC_BYTES_6);
  } else {
    // I2C error: attempt recovery
    if (I2C_BusRecovery() == HAL_OK) {
      // Try to re-initialize IMU
      IMU_Init();
    }
    // Skip this transmission cycle - will retry next loop
  }
>>>>>>> Stashed changes
}




void SendStrainOnCan(uint32_t can_id, uint32_t channel) {
<<<<<<< Updated upstream
        ADC_ChannelConfTypeDef sConfig = {0};
        uint16_t strain_val = 0;

        // 1. USE THE CORRECT HANDLE (hadc2)
        // 2. Add Error Checking
        sConfig.Channel = channel;
        // ... setup ...

        // If using hadc2:
        if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK) return;

        HAL_ADC_Start(&hadc2);
        if (HAL_ADC_PollForConversion(&hadc2, 10) == HAL_OK) {
            strain_val = (uint16_t)HAL_ADC_GetValue(&hadc2);
        }
        HAL_ADC_Stop(&hadc2);

        uint8_t data[2] = { (strain_val >> 8) & 0xFF, strain_val & 0xFF };
        CAN_Transmit(can_id, data, FDCAN_DLC_BYTES_2);
    }

// void SendStrainOnCan(uint32_t can_id, uint32_t channel) {
//     ADC_ChannelConfTypeDef sConfig = {0};
//     uint16_t strain_val = 0;
//
//     // --- FIX STARTS HERE ---
//     // USE hadc2, NOT hadc1
//     sConfig.Channel = channel;
//     sConfig.Rank = ADC_REGULAR_RANK_1;
//     sConfig.SamplingTime = ADC_SAMPLETIME_810CYCLES_5; // Check if ADC2 supports this timing
//     sConfig.SingleDiff = ADC_SINGLE_ENDED;
//
//     // Use hadc2 for all calls here
//     if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK) {
//         // Optional: Add error handling
//     }
//
//     HAL_ADC_Start(&hadc2);
//     if (HAL_ADC_PollForConversion(&hadc2, 10) == HAL_OK) {
//         strain_val = (uint16_t)HAL_ADC_GetValue(&hadc2);
//     }
//     HAL_ADC_Stop(&hadc2);
//     // --- FIX ENDS HERE ---
//
//
// }
=======
  ADC_ChannelConfTypeDef sConfig = {0};
  uint16_t strain_val = 0;

  // Configure ADC1 to the requested Strain Gauge channel
  sConfig.Channel = channel;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_64CYCLES_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  HAL_ADC_ConfigChannel(&hadc1, &sConfig);

  HAL_ADC_Start(&hadc1);
  if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
    strain_val = (uint16_t)HAL_ADC_GetValue(&hadc1);
  }
  HAL_ADC_Stop(&hadc1);

  uint8_t data[2] = {(strain_val >> 8) & 0xFF, strain_val & 0xFF};
  CAN_Transmit(can_id, data, FDCAN_DLC_BYTES_2);
}
>>>>>>> Stashed changes
