/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    i2c.c
  * @brief   This file provides code for the configuration
  *          of the I2C instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "i2c.h"

/* USER CODE BEGIN 0 */

/* IMU I2C Address */
#define ICM42670_I2C_ADDR           (0x68 << 1) // 0xD0

/* IMU Register Map */
#define REG_ACCEL_DATA_X1           0x0B
#define REG_PWR_MGMT0               0x1F
#define REG_COMBINED_CONFIG         0x20 // Gyro and Accel FSR/ODR configuration
#define REG_GYRO_ACCEL_FILT         0x28 // Digital Low-Pass Filter configuration
#define REG_INT_CONFIG              0x14 // Interrupt pin configuration
#define REG_INT_SOURCE0             0x65 // Interrupt source routing
#define INT1_OPEN_DRAIN_ACTIVE_LOW  0x04

/* IMU Configuration Bitmasks */
#define PWR_LN_MODE                 0x0F // Accel and Gyro Low Noise Mode
#define CONFIG_16G_2000DPS_1KHZ     0x4F // Packed FSR (±16g, ±2000 dps) and 1 kHz ODR
#define FILT_BW_25HZ                0x33 // DLPF Cutoff at ~25Hz for both sensors
#define INT1_PUSH_PULL_ACTIVE_HIGH  0x02 // INT1 pin drive configuration
#define INT_SOURCE_DRDY             0x08 // Route Data Ready to INT1

/* USER CODE END 0 */

I2C_HandleTypeDef hi2c1;

/* I2C1 init function */
void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00B03FDB;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

void HAL_I2C_MspInit(I2C_HandleTypeDef* i2cHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
  if(i2cHandle->Instance==I2C1)
  {
  /* USER CODE BEGIN I2C1_MspInit 0 */

  /* USER CODE END I2C1_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2C1;
    PeriphClkInitStruct.I2c123ClockSelection = RCC_I2C1235CLKSOURCE_D2PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**I2C1 GPIO Configuration
    PB6     ------> I2C1_SCL
    PB7     ------> I2C1_SDA
    */
    GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* I2C1 clock enable */
    __HAL_RCC_I2C1_CLK_ENABLE();
  /* USER CODE BEGIN I2C1_MspInit 1 */

  /* USER CODE END I2C1_MspInit 1 */
  }
}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef* i2cHandle)
{

  if(i2cHandle->Instance==I2C1)
  {
  /* USER CODE BEGIN I2C1_MspDeInit 0 */

  /* USER CODE END I2C1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_I2C1_CLK_DISABLE();

    /**I2C1 GPIO Configuration
    PB6     ------> I2C1_SCL
    PB7     ------> I2C1_SDA
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6);

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_7);

  /* USER CODE BEGIN I2C1_MspDeInit 1 */

  /* USER CODE END I2C1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

HAL_StatusTypeDef IMU_Init(void) {
  HAL_StatusTypeDef status;
  uint8_t tx_data;
  uint8_t who_am_i = 0;

  // 0. Bus recovery: toggle SCL 9 times to unstick SDA
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin   = GPIO_PIN_6 | GPIO_PIN_7;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);

  for (int i = 0; i < 9; i++) {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
    HAL_Delay(1);
  }

  // STOP condition
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);
  HAL_Delay(1);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);
  HAL_Delay(1);

  HAL_I2C_DeInit(&hi2c1);

  // Re-init restores PB6/PB7 to AF mode
  MX_I2C1_Init();
  HAL_Delay(5);

  // 1. Verify the IMU is present and responding
  status = HAL_I2C_Mem_Read(&hi2c1, ICM42670_I2C_ADDR, 0x75, I2C_MEMADD_SIZE_8BIT, &who_am_i, 1, 100);
  if (status != HAL_OK || who_am_i != 0x67) return HAL_ERROR;

  // 2. Power Management: Wake up and enter Low Noise Mode
  tx_data = 0x0F;
  status = HAL_I2C_Mem_Write(&hi2c1, ICM42670_I2C_ADDR, 0x1F, I2C_MEMADD_SIZE_8BIT, &tx_data, 1, 100);
  if (status != HAL_OK) return status;

  HAL_Delay(10);

  // 3. Gyro Config
  tx_data = 0x04;
  status = HAL_I2C_Mem_Write(&hi2c1, ICM42670_I2C_ADDR, 0x20, I2C_MEMADD_SIZE_8BIT, &tx_data, 1, 100);
  if (status != HAL_OK) return status;

  // 4. Accel Config
  tx_data = 0x24;
  status = HAL_I2C_Mem_Write(&hi2c1, ICM42670_I2C_ADDR, 0x21, I2C_MEMADD_SIZE_8BIT, &tx_data, 1, 100);
  if (status != HAL_OK) return status;

  // 5. Interrupt Configuration: Latched, Open-Drain, Active Low
  tx_data = 0x00;
  status = HAL_I2C_Mem_Write(&hi2c1, ICM42670_I2C_ADDR, 0x14, I2C_MEMADD_SIZE_8BIT, &tx_data, 1, 100);
  if (status != HAL_OK) return status;

  // 6. Interrupt Routing: INT_SOURCE0 (0x65) is in MREG1, requires indirect write
  // Step A: Select MREG1 bank
  tx_data = 0x00;
  status = HAL_I2C_Mem_Write(&hi2c1, ICM42670_I2C_ADDR, 0x7C, I2C_MEMADD_SIZE_8BIT, &tx_data, 1, 100);
  if (status != HAL_OK) return status;

  // Step B: Set target register address
  tx_data = 0x65;
  status = HAL_I2C_Mem_Write(&hi2c1, ICM42670_I2C_ADDR, 0x7D, I2C_MEMADD_SIZE_8BIT, &tx_data, 1, 100);
  if (status != HAL_OK) return status;

  // Step C: Write DRDY_INT1_EN
  tx_data = 0x08;
  status = HAL_I2C_Mem_Write(&hi2c1, ICM42670_I2C_ADDR, 0x7E, I2C_MEMADD_SIZE_8BIT, &tx_data, 1, 100);
  if (status != HAL_OK) return status;

  HAL_Delay(1);

  // 7. Dummy read to clear any pending interrupt and arm the latch
  uint8_t dummy[12];
  HAL_I2C_Mem_Read(&hi2c1, ICM42670_I2C_ADDR, REG_ACCEL_DATA_X1, I2C_MEMADD_SIZE_8BIT, dummy, 12, 100);

  return HAL_OK;
}

/* USER CODE END 1 */

