#include "control.h"
#include <string.h>

// Hardware Handles
extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;
extern FDCAN_HandleTypeDef hfdcan1;
extern I2C_HandleTypeDef hi2c1;
extern SPI_HandleTypeDef hspi1;

// Global variables for CLion Live Watches
volatile float live_gyro_x = 0.0f;
volatile float live_gyro_y = 0.0f;
volatile float live_gyro_z = 0.0f;
volatile float live_accel_x = 0.0f;
volatile float live_accel_y = 0.0f;
volatile float live_accel_z = 0.0f;
volatile float live_pot = 0.0f;

#define GYRO_SENSITIVITY_2000DPS 16.4f
#define ACCEL_SENSITIVITY_16G 2048.0f
#define I2C_RECOVERY_MAX_CLOCKS 9

// Global state
static uint8_t g_imu_initialized = 0;
static uint8_t a_imu_initialized = 0;
uint8_t pot_tx_data[2];

// Debug variables for Live Expression Watch
volatile HAL_StatusTypeDef g_debug_i2c_status = HAL_OK;
volatile uint32_t g_debug_can_tx_errors = 0;
volatile uint32_t g_debug_i2c_recoveries = 0;

volatile HAL_StatusTypeDef a_debug_i2c_status = HAL_OK;
volatile uint32_t a_debug_can_tx_errors = 0;
volatile uint32_t a_debug_i2c_recoveries = 0;

const StepperSetting_t suspension_profiles[4] = {
    {0,    0},     // Setting 0: Full Soft / Home
    {450,  600},   // Setting 1: Profile 1 (N17 moves 450, N23 moves 600)
    {1200, 300},   // Setting 2: Profile 2
    {2000, 1800}   // Setting 3: Full Hard / Max Extension
};

volatile int32_t nema17_current_pos = 0;
volatile int32_t nema23_current_pos = 0;

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


  // Wait for space in TX FIFO if necessary
  while (HAL_FDCAN_GetTxFifoFreeLevel(&hfdcan1) == 0) {
    // Optional: add a timeout here if you don't want to block forever
    // For now, we block to ensure message delivery
  }

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
}

/**
 * @brief Initialize ICM-42670-P IMU with retry and recovery
 * @return HAL_OK on success, HAL_ERROR if IMU not responding after retries
 */
HAL_StatusTypeDef IMU_Init(void) {
  uint8_t check = 0;
  uint8_t data = 0;
  HAL_StatusTypeDef status;

  g_imu_initialized = 0;
  a_imu_initialized = 0;

  // Try initialization with retries
  // Try initialization with retries
  for (int retry = 0; retry < IMU_INIT_RETRY_COUNT; retry++) {
    // Check WHO_AM_I with short timeout
    status = HAL_I2C_Mem_Read(&hi2c1, ICM_ADDR, WHO_AM_I, I2C_MEMADD_SIZE_8BIT,
                              &check, 1, 50);

    if (status == HAL_OK && check == WHO_AM_I_VAL) {
      // 1. Configure Gyroscope: +/- 2000 dps, ODR = 100 Hz
      data = 0x07;
      status = HAL_I2C_Mem_Write(&hi2c1, ICM_ADDR, GYRO_CONFIG0,
                                 I2C_MEMADD_SIZE_8BIT, &data, 1, 50);
      if (status != HAL_OK) continue;

      // 2. NEW: Configure Accelerometer: +/- 16g, ODR = 100 Hz
      // FS (6:5): 00 = 16g, ODR (3:0): 0111 = 100Hz
      data = 0x07;
      status = HAL_I2C_Mem_Write(&hi2c1, ICM_ADDR, ACCEL_CONFIG0,
                                 I2C_MEMADD_SIZE_8BIT, &data, 1, 50);
      if (status != HAL_OK) continue;

      // 3. UPDATED Power Management: Enable BOTH Accel and Gyro in Low Noise mode
      // ACCEL_MODE (1:0): 11 (LN), GYRO_MODE (3:2): 11 (LN) -> 1111 = 0x0F
      data = 0x0F;
      status = HAL_I2C_Mem_Write(&hi2c1, ICM_ADDR, PWR_MGMT0,
                                 I2C_MEMADD_SIZE_8BIT, &data, 1, 50);
      if (status != HAL_OK) continue;

      // Wait for sensors to stabilize after power mode change
      HAL_Delay(IMU_STARTUP_DELAY_MS);

      g_imu_initialized = 1;
      a_imu_initialized = 1;
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
      float voltage = ((float)adcRaw * 1.8f) / 65535.0f;
      float position = VoltageToPosition(voltage);
      posSum += position;
    }
    HAL_ADC_Stop(&hadc1);
  }

  float posAvg = posSum / samples;
  live_pot = posAvg;
  uint16_t posCan = (uint16_t)(posAvg * 100.0f);
  pot_tx_data[0] = (posCan >> 8) & 0xFF;
  pot_tx_data[1] = posCan & 0xFF;

  CAN_Transmit(can_id, pot_tx_data, FDCAN_DLC_BYTES_2);
}

void SendGyroOnCan(uint32_t can_id) {
  uint8_t raw_data[6] = {0};

  if (!g_imu_initialized) return;

  if (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY) {
    HAL_I2C_Master_Abort_IT(&hi2c1, ICM_ADDR);
    HAL_Delay(5);
    if (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY) {
      I2C_BusRecovery();
      IMU_Init();
      return;
    }
  }

  g_debug_i2c_status = HAL_I2C_Mem_Read(&hi2c1, ICM_ADDR, GYRO_DATA_START,
                                        I2C_MEMADD_SIZE_8BIT, raw_data, 6, 50);

  if (g_debug_i2c_status == HAL_OK) {
    // Convert to signed 16-bit for CLion Live Watch
    int16_t x = (int16_t)((raw_data[0] << 8) | raw_data[1]);
    int16_t y = (int16_t)((raw_data[2] << 8) | raw_data[3]);
    int16_t z = (int16_t)((raw_data[4] << 8) | raw_data[5]);

    live_gyro_x = (float)x / GYRO_SENSITIVITY_2000DPS;
    live_gyro_y = (float)y / GYRO_SENSITIVITY_2000DPS;
    live_gyro_z = (float)z / GYRO_SENSITIVITY_2000DPS;

    CAN_Transmit(can_id, raw_data, FDCAN_DLC_BYTES_6);
  } else {
    if (I2C_BusRecovery() == HAL_OK) IMU_Init();
  }
}

void SendAccelOnCan(uint32_t can_id) {
  uint8_t raw_data[6] = {0};

  if (!a_imu_initialized) return;

  if (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY) {
    HAL_I2C_Master_Abort_IT(&hi2c1, ICM_ADDR);
    HAL_Delay(5);
    if (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY) {
      I2C_BusRecovery();
      IMU_Init();
      return;
    }
  }

  a_debug_i2c_status = HAL_I2C_Mem_Read(&hi2c1, ICM_ADDR, ACCEL_DATA_START,
                                        I2C_MEMADD_SIZE_8BIT, raw_data, 6, 50);

  if (a_debug_i2c_status == HAL_OK) {
    // Convert to signed 16-bit for CLion Live Watch
    int16_t x = (int16_t)((raw_data[0] << 8) | raw_data[1]);
    int16_t y = (int16_t)((raw_data[2] << 8) | raw_data[3]);
    int16_t z = (int16_t)((raw_data[4] << 8) | raw_data[5]);

    live_accel_x = (float)x / ACCEL_SENSITIVITY_16G;
    live_accel_y = (float)y / ACCEL_SENSITIVITY_16G;
    live_accel_z = (float)z / ACCEL_SENSITIVITY_16G;

    CAN_Transmit(can_id, raw_data, FDCAN_DLC_BYTES_6);
  } else {
    if (I2C_BusRecovery() == HAL_OK) IMU_Init();
  }
}

/**
 * @brief SPI Transfer for a specific motor
 */
uint16_t DRV8461_Transfer(MotorID_t motor, uint8_t addr, uint8_t data) {
  uint16_t tx_frame = ((uint16_t)addr << 8) | data;
  uint16_t rx_frame = 0;

  GPIO_TypeDef* ncs_port;
  uint16_t ncs_pin;

  // Select the correct Chip Select pin
  if (motor == MOTOR_NEMA17) {
    ncs_port = N17_NCS_PORT; ncs_pin = N17_NCS_PIN;
  } else {
    ncs_port = N23_NCS_PORT; ncs_pin = N23_NCS_PIN;
  }

  HAL_GPIO_WritePin(ncs_port, ncs_pin, GPIO_PIN_RESET); // Select
  HAL_SPI_TransmitReceive(&hspi1, (uint8_t*)&tx_frame, (uint8_t*)&rx_frame, 1, 10);
  HAL_GPIO_WritePin(ncs_port, ncs_pin, GPIO_PIN_SET);   // Deselect

  return rx_frame;
}

/**
 * @brief Microsecond delay using DWT cycle counter
 */
void Delay_us(uint32_t us) {
    uint32_t startTick = DWT->CYCCNT;
    uint32_t delayTicks = us * (SystemCoreClock / 1000000);

    while (DWT->CYCCNT - startTick < delayTicks);
}

void Motors_Init(void) {
  // Enable DWT Cycle Counter for microsecond delays
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->LAR = 0xC5ACCE55;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

  // 1. Wake up the DRV8461 Logic ONLY (Keep power outputs DISABLED)
  HAL_GPIO_WritePin(N17_SLEEP_PORT, N17_SLEEP_PIN, GPIO_PIN_SET);
  HAL_GPIO_WritePin(N23_SLEEP_PORT, N23_SLEEP_PIN, GPIO_PIN_SET);

  HAL_GPIO_WritePin(N17_ENABLE_PORT, N17_ENABLE_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(N23_ENABLE_PORT, N23_ENABLE_PIN, GPIO_PIN_RESET);

  HAL_Delay(10); // Wait for driver logic to boot

  // 2. Configure SPI Parameters while outputs are safely off
  
  // In Motors_Init():
  DRV8461_Transfer(MOTOR_NEMA17, DRV_REG_CTRL1, 0x9F);  // Max TOFF
  DRV8461_Transfer(MOTOR_NEMA23, DRV_REG_CTRL1, 0x9F);

  DRV8461_Transfer(MOTOR_NEMA17, DRV_REG_CTRL2, 0x05);  // 1/32 microstepping
  DRV8461_Transfer(MOTOR_NEMA23, DRV_REG_CTRL2, 0x05);

  DRV8461_Transfer(MOTOR_NEMA17, DRV_REG_CTRL3, 0x09);  // 60% torque N17
  DRV8461_Transfer(MOTOR_NEMA23, DRV_REG_CTRL3, 0x0F);  // 100% torque N23

  DRV8461_Transfer(MOTOR_NEMA17, DRV_REG_CTRL4, 0x00);  // Auto SmartTune
  DRV8461_Transfer(MOTOR_NEMA23, DRV_REG_CTRL4, 0x00);

  // CTRL6 (0x09): Enable spread spectrum dithering
  // Bits 5:4 = SSC_EN, Bits 3:2 = dither amplitude
  DRV8461_Transfer(MOTOR_NEMA17, DRV_REG_CTRL6, 0x30); // Enable SSC, moderate dither
  DRV8461_Transfer(MOTOR_NEMA23, DRV_REG_CTRL6, 0x30);

  // CTRL9 (0x0C): 0x10 -> Enable Stall Detection
  DRV8461_Transfer(MOTOR_NEMA17, DRV_REG_CTRL9, 0x10);
  DRV8461_Transfer(MOTOR_NEMA23, DRV_REG_CTRL9, 0x10);

  // CTRL10 (0x0D): 0x0F -> Full current scale
  DRV8461_Transfer(MOTOR_NEMA17, DRV_REG_CTRL10, 0x0F);
  DRV8461_Transfer(MOTOR_NEMA23, DRV_REG_CTRL10, 0x0F);

  // 3. Staggered Power Enable (Prevent Brown-Out)
  HAL_GPIO_WritePin(N17_ENABLE_PORT, N17_ENABLE_PIN, GPIO_PIN_SET); // Energize N17
  HAL_Delay(150); // Let power supply voltage stabilize

  HAL_GPIO_WritePin(N23_ENABLE_PORT, N23_ENABLE_PIN, GPIO_PIN_SET); // Energize N23
  HAL_Delay(150); // Let power supply voltage stabilize
}


void Motor_Step(MotorID_t motor, uint8_t direction, uint32_t steps) {
  GPIO_TypeDef* step_port; uint16_t step_pin;
  GPIO_TypeDef* dir_port;  uint16_t dir_pin;

  if (motor == MOTOR_NEMA17) {
    step_port = N17_STEP_PORT; step_pin = N17_STEP_PIN;
    dir_port = N17_DIR_PORT;   dir_pin = N17_DIR_PIN;
  } else {
    step_port = N23_STEP_PORT; step_pin = N23_STEP_PIN;
    dir_port = N23_DIR_PORT;   dir_pin = N23_DIR_PIN;
  }

  HAL_GPIO_WritePin(dir_port, dir_pin, direction ? GPIO_PIN_SET : GPIO_PIN_RESET);
  Delay_us(50); // Setup time

  uint32_t min_delay = 500;  // 1kHz max speed
  uint32_t max_delay = 2000; // 250Hz start speed
  uint32_t accel_steps = (steps > 100) ? 50 : steps / 2;

  for(uint32_t i = 0; i < steps; i++) {
    uint32_t current_delay = max_delay;

    if (i < accel_steps) {
        // Accelerate
        current_delay = max_delay - ((max_delay - min_delay) * i / accel_steps);
    } else if (i > steps - accel_steps) {
        // Decelerate
        current_delay = min_delay + ((max_delay - min_delay) * (i - (steps - accel_steps)) / accel_steps);
    } else {
        current_delay = min_delay;
    }

    HAL_GPIO_WritePin(step_port, step_pin, GPIO_PIN_SET);
    Delay_us(10); // Pulse width
    HAL_GPIO_WritePin(step_port, step_pin, GPIO_PIN_RESET);
    Delay_us(current_delay);
  }
}

void Motors_Step_Simultaneous(int32_t move17, int32_t move23) {
  uint8_t dir17 = (move17 > 0);
  uint8_t dir23 = (move23 > 0);
  uint32_t steps17 = (move17 > 0) ? move17 : -move17;
  uint32_t steps23 = (move23 > 0) ? move23 : -move23;

  HAL_GPIO_WritePin(N17_DIR_PORT, N17_DIR_PIN, dir17 ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(N23_DIR_PORT, N23_DIR_PIN, dir23 ? GPIO_PIN_SET : GPIO_PIN_RESET);

  Delay_us(50);

  uint32_t max_steps = (steps17 > steps23) ? steps17 : steps23;

  uint32_t min_delay = 500;
  uint32_t max_delay = 2000;
  uint32_t accel_steps = (max_steps > 100) ? 50 : max_steps / 2;

  for (uint32_t i = 0; i < max_steps; i++) {
    uint32_t current_delay = max_delay;

    if (i < accel_steps) {
        current_delay = max_delay - ((max_delay - min_delay) * i / accel_steps);
    } else if (i > max_steps - accel_steps) {
        current_delay = min_delay + ((max_delay - min_delay) * (i - (max_steps - accel_steps)) / accel_steps);
    } else {
        current_delay = min_delay;
    }

    // Pulse N17
    if (i < steps17) {
      HAL_GPIO_WritePin(N17_STEP_PORT, N17_STEP_PIN, GPIO_PIN_SET);
    }

    // Pulse N23
    if (i < steps23) {
      HAL_GPIO_WritePin(N23_STEP_PORT, N23_STEP_PIN, GPIO_PIN_SET);
    }

    Delay_us(10);
    HAL_GPIO_WritePin(N17_STEP_PORT, N17_STEP_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(N23_STEP_PORT, N23_STEP_PIN, GPIO_PIN_RESET);

    Delay_us(current_delay);
  }
}

void Motor_GoTo_Setting(uint8_t setting_id) {
    if (setting_id > 3) return;

    StepperSetting_t target = suspension_profiles[setting_id];

    // Calculate displacement for both
    int32_t diff17 = target.nema17_target - nema17_current_pos;
    int32_t diff23 = target.nema23_target - nema23_current_pos;

    // If either needs to move, move them together
    if (diff17 != 0 || diff23 != 0) {
        Motors_Step_Simultaneous(diff17, diff23);

        // Update trackers to the final profile targets
        nema17_current_pos = target.nema17_target;
        nema23_current_pos = target.nema23_target;
    }
}

void Motor_Calibrate_All(void) {
  // Reset Indexers via SPI to clear any lingering faults from the whine
  DRV8461_Transfer(MOTOR_NEMA17, DRV_REG_CTRL1, 0xA7);
  DRV8461_Transfer(MOTOR_NEMA23, DRV_REG_CTRL1, 0xA7);
  HAL_Delay(5);

  // Force directions to backward (0)
  HAL_GPIO_WritePin(N17_DIR_PORT, N17_DIR_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(N23_DIR_PORT, N23_DIR_PIN, GPIO_PIN_RESET);
  HAL_Delay(5);

  // Dedicated SLOW home for NEMA 17 to overcome mechanical inertia
  for (uint32_t i = 0; i < 3000; i++) {
    HAL_GPIO_WritePin(N17_STEP_PORT, N17_STEP_PIN, GPIO_PIN_SET);
    Delay_us(10);
    HAL_GPIO_WritePin(N17_STEP_PORT, N17_STEP_PIN, GPIO_PIN_RESET);
    Delay_us(3000); // 3ms delay (~333 Hz)
  }

  // Dedicated SLOW home for NEMA 23
  for (uint32_t i = 0; i < 3000; i++) {
    HAL_GPIO_WritePin(N23_STEP_PORT, N23_STEP_PIN, GPIO_PIN_SET);
    Delay_us(10);
    HAL_GPIO_WritePin(N23_STEP_PORT, N23_STEP_PIN, GPIO_PIN_RESET);
    Delay_us(3000);
  }

  // Reset trackers
  nema17_current_pos = 0;
  nema23_current_pos = 0;
}

/**
 * @brief Checks for motor faults and attempts a recovery if needed
 * @return 1 if a recovery was performed, 0 if everything is OK
 */
uint8_t Motor_CheckAndRecover(void) {
    uint8_t faulted = 0;

    // Check N17 Fault (Active Low)
    if (HAL_GPIO_ReadPin(N17_FAULT_PORT, N17_FAULT_PIN) == GPIO_PIN_RESET) {
        faulted = 1;
        // Toggle SLEEP or ENABLE to clear latching faults
        HAL_GPIO_WritePin(N17_ENABLE_PORT, N17_ENABLE_PIN, GPIO_PIN_RESET);
        HAL_Delay(10);
        HAL_GPIO_WritePin(N17_ENABLE_PORT, N17_ENABLE_PIN, GPIO_PIN_SET);

        // Re-send SPI Config (Some faults wipe volatile registers)
        DRV8461_Transfer(MOTOR_NEMA17, DRV_REG_CTRL1, 0x87);
    }

    // Check N23 Fault
    if (HAL_GPIO_ReadPin(N23_FAULT_PORT, N23_FAULT_PIN) == GPIO_PIN_RESET) {
        faulted = 1;
        HAL_GPIO_WritePin(N23_ENABLE_PORT, N23_ENABLE_PIN, GPIO_PIN_RESET);
        HAL_Delay(10);
        HAL_GPIO_WritePin(N23_ENABLE_PORT, N23_ENABLE_PIN, GPIO_PIN_SET);

        DRV8461_Transfer(MOTOR_NEMA23, DRV_REG_CTRL1, 0x87);
    }

    if (faulted) {
        // Wait 1 second before allowing the motor to spin again
        // This prevents a "vicious cycle" of instant re-faulting
        HAL_Delay(1000);
    }

    return faulted;
}

/**
 * @brief Reads motor driver status and broadcasts over CAN
 * Payload Structure (8 Bytes):
 * Byte 0: NEMA17 Hardware Status (0=OK, 1=Fault/Short)
 * Byte 1: NEMA17 SPI Status (Bit 0: Stall, Bit 1: Over-temp)
 * Byte 2: NEMA23 Hardware Status (0=OK, 1=Fault/Short)
 * Byte 3: NEMA23 SPI Status (Bit 0: Stall, Bit 1: Over-temp)
 */
void SendEsusStatusOnCan(uint32_t can_id) {
    uint8_t status_msg[8] = {0};

    // Check Hardware Pins (Active Low)
    status_msg[0] = (HAL_GPIO_ReadPin(N17_FAULT_PORT, N17_FAULT_PIN) == GPIO_PIN_RESET) ? 1 : 0;
    status_msg[2] = (HAL_GPIO_ReadPin(N23_FAULT_PORT, N23_FAULT_PIN) == GPIO_PIN_RESET) ? 1 : 0;

    // Query SPI Registers for detailed diagnostics
    // We read DIAG2 (0x02) which contains Stall and Temp info
    uint16_t n17_diag = DRV8461_Transfer(MOTOR_NEMA17, (0x80 | DRV_REG_DIAG2), 0x00); // 0x80 is READ bit
    uint16_t n23_diag = DRV8461_Transfer(MOTOR_NEMA23, (0x80 | DRV_REG_DIAG2), 0x00);

    // Byte 1 & 3: Map SPI bits to our CAN payload
    // Bit 3 of DIAG2 is STALL, Bit 6 is OTW (Over-temp warning)
    status_msg[1] = (uint8_t)(n17_diag & 0xFF);
    status_msg[3] = (uint8_t)(n23_diag & 0xFF);

    // Broadcast
    CAN_Transmit(can_id, status_msg, FDCAN_DLC_BYTES_8);
}

// void SendStrainOnCan(uint32_t can_id, uint32_t channel) {
//   ADC_ChannelConfTypeDef sConfig = {0};
//   uint16_t strain_val = 0;
//
//   // Configure ADC1 to the requested Strain Gauge channel
//   sConfig.Channel = channel;
//   sConfig.Rank = ADC_REGULAR_RANK_1;
//   sConfig.SamplingTime = ADC_SAMPLETIME_64CYCLES_5;
//   sConfig.SingleDiff = ADC_SINGLE_ENDED;
//   sConfig.OffsetNumber = ADC_OFFSET_NONE;
//   sConfig.Offset = 0;
//   HAL_ADC_ConfigChannel(&hadc1, &sConfig);
//
//   HAL_ADC_Start(&hadc1);
//   if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
//     strain_val = (uint16_t)HAL_ADC_GetValue(&hadc1);
//   }
//   HAL_ADC_Stop(&hadc1);
//
//   uint8_t data[2] = {(strain_val >> 8) & 0xFF, strain_val & 0xFF};
//   CAN_Transmit(can_id, data, FDCAN_DLC_BYTES_2);
// }
