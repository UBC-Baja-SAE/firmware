/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "CO_app_STM32.h"
#include "fdcan.h"
#include "tim.h"
#include "OD.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
extern CANopenNodeSTM32 canOpenNodeSTM32;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

extern volatile uint32_t linpot_raw_value;
extern volatile int16_t  imu_accel_x, imu_accel_y, imu_accel_z;
extern volatile int16_t  imu_gyro_x,  imu_gyro_y,  imu_gyro_z;

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 2048 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for canopen */
osThreadId_t canopenHandle;
const osThreadAttr_t canopen_attributes = {
  .name = "canopen",
  .stack_size = 2048 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

void ICM42670_Init(void);
void ICM42670_ReadData(void);

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void canopen_task(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of canopen */
  canopenHandle = osThreadNew(canopen_task, NULL, &canopen_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */

  // Constants for sensor scaling
  const float GRAVITY_MSS = 9.80665f;
  const float DEG_TO_RAD  = 0.0174533f;
  const float ACCEL_SCALE = (1.0f / 4096.0f) * GRAVITY_MSS;
  const float GYRO_SCALE  = (1.0f / 16.4f)   * DEG_TO_RAD;

  // Wait briefly for CANopen to initialize before hammering the I2C/ADC
  osDelay(500);

  /* Infinite loop */
  for(;;)
  {
    // 1. Read raw data from the IMU via I2C
    ICM42670_ReadData();

    // 2. Scale the linear potentiometer
    float linpot_scaled = 36.5f + ((float)linpot_raw_value / 4095.0f) * 25.0f;

    // 3. Write directly to the CANopen Object Dictionary
    // CANopenNode automatically locks/unlocks the OD during PDO transmission,
    // but writing basic 32-bit floats here is generally thread-safe on a 32-bit MCU.

    // Write Linpot (Assuming OD_RAM generated it as a uint32_t, we cast the float bits)
    // If OD_RAM generated it as a float32_t, just do OD_RAM.x2000_linearPotentiometer = linpot_scaled;
    OD_RAM.x2000_linearPotentiometer = linpot_scaled;

    // Write IMU Data (Assuming OD.h typed x2001_imu as float32_t array based on your REAL32 definition)
    OD_RAM.x2001_imu[0] = (float)imu_accel_x * ACCEL_SCALE;
    OD_RAM.x2001_imu[1] = (float)imu_accel_y * ACCEL_SCALE;
    OD_RAM.x2001_imu[2] = (float)imu_accel_z * ACCEL_SCALE;
    OD_RAM.x2001_imu[3] = (float)imu_gyro_x  * GYRO_SCALE;
    OD_RAM.x2001_imu[4] = (float)imu_gyro_y  * GYRO_SCALE;
    OD_RAM.x2001_imu[5] = (float)imu_gyro_z  * GYRO_SCALE;

    // Run this task at roughly 20Hz (50ms)
    osDelay(50);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_canopen_task */
/**
* @brief Function implementing the canopen thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_canopen_task */
void canopen_task(void *argument)
{
  /* USER CODE BEGIN canopen_task */
  canOpenNodeSTM32.CANHandle = &hfdcan1;
  canOpenNodeSTM32.HWInitFunction = MX_FDCAN1_Init;
  canOpenNodeSTM32.timerHandle = &htim6;   // your 1ms timer
  canOpenNodeSTM32.desiredNodeID = 1;
  canOpenNodeSTM32.baudrate = 500;
  canopen_app_init(&canOpenNodeSTM32);


  /* Infinite loop */
  for(;;)
  {

    canopen_app_process();
    osDelay(1);
  }
  /* USER CODE END canopen_task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

