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
#include "FreeRTOS.h"
#include "cmsis_os2.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>

#include <stdint.h>
#include "helpers.h"

#include "fdcan.h"

#include "roxy.h"
#include "SEGGER_RTT.h"
#include "st7735.h"
#include "FreeRTOSConfig.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef StaticTask_t osStaticThreadDef_t;
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

// i2c polling addresses based on GY-906 datasheet
#define IR_SLAVE_ADDR (0x5A)
#define AMB_POLL_ADDR (0x06)
#define OBJ_POLL_ADDR (0x07)

#define BYTE_SHIFT (8u)

// delta calculated based on accepted celsius diff divided by sensor resolution
// 10 degrees celsius divided by 0.02 resolution
#define CVT_ACCEPTED_DELTA (500u)
// Ticks required for sense state to accept total change in readings
#define SENSE_STATE_FLIP (15u)
// EMA uses the following defines to allocate 25% to latest reading and 75% to history
#define DIV_BY_FOUR_IN_BITS (2u)
#define TEMP_HIST_FACTOR (3u)
#define INT_ROUND_FACTOR (2u)

// buffer sizing defines
#define IR_POLL_BFR_SIZE (3u)
#define CAN_TRANSMIT_TICKS    (100)

#define SETTLE_TICKS          (200)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

osThreadId_t thermalTaskHandle;
const osThreadAttr_t thermalTask_attributes = {
  .name = "thermalTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

osSemaphoreId_t I2C1_Rx_SemHandle;

// CANDFD GLOBAL STRUCT DEF
typedef struct __attribute__((packed)) {
  thermo_read_can_t thermo_read_can; // 4 bytes
  uint8_t padding[60];
} can_global_transmit_t;

// Sensor Read Structs
typedef struct __attribute__((packed)) {
  uint8_t lsb;  // i2c reads for IR sensor come in 2 bytes
  uint8_t msb;
  uint8_t crc;  // checksum for valid data
} thermo_read_raw_t;

// Processed obj and amb readings for CAN transmit
typedef struct __attribute__((packed)) {
  uint8_t obj_lsb;
  uint8_t obj_msb;
  uint8_t amb_lsb;
  uint8_t amb_msb;
} thermo_read_can_t;

// Single Variables
can_global_transmit_t can_global_transmit = {0};
volatile thermo_read_raw_t obj_read_raw = {0,0,0};
volatile thermo_read_raw_t amb_read_raw = {0,0,0};
volatile thermo_read_can_t ir_read_can = {0,0,0,0};

extern FDCAN_HandleTypeDef hfdcan1;

uint16_t prev_obj_raw = 0;
uint16_t obj_hist = 0;
uint8_t base_sense = 0;

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for CANFDTransmTask */
osThreadId_t CANFDTransmTaskHandle;
uint32_t CANFDTransmitTaskBuffer[ 128 ];
osStaticThreadDef_t CANFDTransmitTaskControlBlock;
const osThreadAttr_t CANFDTransmTask_attributes = {
  .name = "CANFDTransmTask",
  .cb_mem = &CANFDTransmitTaskControlBlock,
  .cb_size = sizeof(CANFDTransmitTaskControlBlock),
  .stack_mem = &CANFDTransmitTaskBuffer[0],
  .stack_size = sizeof(CANFDTransmitTaskBuffer),
  .priority = (osPriority_t) osPriorityHigh,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void StartThermalSensorTask(void *argument);

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void StartCANFDTransmitTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  // SystemView Init
  SEGGER_SYSVIEW_Conf();

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* creation of I2C1_Rx_Sem */
  // Max count 1, initial count 0 (starts empty so the task blocks immediately)
  I2C1_Rx_SemHandle = osSemaphoreNew(1, 0, NULL);

  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  osTimerStart(EnableCANTxHandle, CAN_TRANSMIT_TICKS);
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of CANFDTransmTask */
  CANFDTransmTaskHandle = osThreadNew(StartCANFDTransmitTask, NULL, &CANFDTransmTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  thermalTaskHandle = osThreadNew(StartThermalSensorTask, NULL, &thermalTask_attributes);
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


  // SystemView Start
  osDelay(SETTLE_TICKS);

  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; // Enable Trace
  DWT->CYCCNT = 0;                                // Reset Cycle Counter
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

  SEGGER_SYSVIEW_Start();

  /* Infinite loop */
  for(;;)
  {
    osDelay(100);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartCANFDTransmitTask */
/**
* @brief Function implementing the CANFDTransmTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartCANFDTransmitTask */
void StartCANFDTransmitTask(void *argument)
{
  /* USER CODE BEGIN StartCANFDTransmitTask */
  FDCAN_TxHeaderTypeDef txHeader;

  txHeader.Identifier = 0x123;
  txHeader.IdType = FDCAN_STANDARD_ID;
  txHeader.TxFrameType = FDCAN_DATA_FRAME;
  txHeader.DataLength = FDCAN_DLC_BYTES_64;
  txHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
  txHeader.BitRateSwitch = FDCAN_BRS_ON;    // Speeds up the clock during the data payload
  txHeader.FDFormat = FDCAN_FD_CAN;         // Enables the up-to-64-byte frame format
  txHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
  txHeader.MessageMarker = 0; // Used if you need to track this specific frame later


  /* Infinite loop */
  for(;;)
  {
    // Copy prepared IR readings onto the CAN buffer using hardware blocking gates
    taskENTER_CRITICAL();
    can_global_transmit.thermo_read_can.obj_lsb = ir_read_can.obj_lsb;
    can_global_transmit.thermo_read_can.obj_msb = ir_read_can.obj_msb;
    can_global_transmit.thermo_read_can.amb_lsb = ir_read_can.amb_lsb;
    can_global_transmit.thermo_read_can.amb_msb = ir_read_can.amb_msb;
    taskEXIT_CRITICAL();

    // Push the 64-byte struct to the CAN FD hardware Tx FIFO
    if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &txHeader, (uint8_t*)&can_global_transmit) != HAL_OK)
    {
      // Optional: Handle transmission error (e.g., Tx FIFO is full)
      // Error_Handler();
    }

    osDelay(100);
  }
  /* USER CODE END StartCANFDTransmitTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

extern I2C_HandleTypeDef hi2c1; // Assuming main I2C refers to hi2c1

/**
* @brief Function implementing the ThermalSensorTask thread.
* @param argument: Not used
* @retval None
*/
void StartThermalSensorTask(void *argument) {
  for(;;) {
    // Send
    HAL_I2C_Mem_Read_DMA(&hi2c1, (IR_SLAVE_ADDR << 1), OBJ_POLL_ADDR, I2C_MEMADD_SIZE_8BIT, (uint8_t*)&obj_read_raw, IR_POLL_BFR_SIZE);
    osSemaphoreAcquire(I2C1_Rx_SemHandle, osWaitForever);

    HAL_I2C_Mem_Read_DMA(&hi2c1, (IR_SLAVE_ADDR << 1), AMB_POLL_ADDR, I2C_MEMADD_SIZE_8BIT, (uint8_t*)&amb_read_raw, IR_POLL_BFR_SIZE);
    osSemaphoreAcquire(I2C1_Rx_SemHandle, osWaitForever);

    // OBJ Handling
    if(MLX90614_VerifyData(OBJ_POLL_ADDR, (volatile uint8_t *)&obj_read_raw)) {
      //CRC Check Passed -- Proceed with Data Processing
      uint16_t obj_read_val = obj_read_raw.msb << 8 | obj_read_raw.lsb;
      uint16_t obj_diff = (obj_read_val > prev_obj_raw) ? obj_read_val - prev_obj_raw : prev_obj_raw - obj_read_val;

      if (obj_diff < CVT_ACCEPTED_DELTA) {
        // Delta indicates allow reading
        if (obj_hist == 0) {
          obj_hist = obj_read_val;
          prev_obj_raw = obj_read_val;
        }

        uint16_t ema = (INT_ROUND_FACTOR + obj_read_val + TEMP_HIST_FACTOR*obj_hist)>>DIV_BY_FOUR_IN_BITS;
        taskENTER_CRITICAL();
        ir_read_can.obj_lsb = (uint8_t)(ema & 0xFF);
        ir_read_can.obj_msb = (uint8_t)(ema >> BYTE_SHIFT);
        taskEXIT_CRITICAL();

        obj_hist = ema;
        prev_obj_raw = obj_read_val;
        base_sense = 0;
      }
      else {
        // Delta indicates that a sensor failure has occurred
        // Relying on previous acceptable reading
        base_sense += 1;

        if (base_sense >= SENSE_STATE_FLIP) {
          prev_obj_raw = obj_read_val;
          base_sense = 0;
        }
      }

    }
    else {
      // crc check failed -- do nothing
    }

    // AMB Handling
    if(MLX90614_VerifyData(AMB_POLL_ADDR, (volatile uint8_t *)&amb_read_raw)) {
      //CRC Check Passed -- Proceed with Data Processing
      taskENTER_CRITICAL();
      ir_read_can.amb_lsb = amb_read_raw.lsb;
      ir_read_can.amb_msb = amb_read_raw.msb;
      taskEXIT_CRITICAL();
    }
    else {
      // crc check failed -- do nothing
    }

    osDelay(1000);
  }
}

/**
* @brief Function releasing sempahore hold of the
* @param hi2c:
* @retval None
*/
void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c) {
  if (hi2c->Instance == I2C1) {
    // Release the semaphore to wake up the Thermal Task
    osSemaphoreRelease(I2C1_Rx_SemHandle);
  }
}

/* USER CODE END Application */

