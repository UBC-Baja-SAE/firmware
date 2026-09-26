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

#include "roxy.h"
#include "SEGGER_RTT.h"
#include "st7735.h"
#include "FreeRTOSConfig.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef StaticTask_t osStaticThreadDef_t;
typedef StaticQueue_t osStaticMessageQDef_t;
typedef StaticEventGroup_t osStaticEventGroupDef_t;
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

// i2c polling addresses based on GY-906 datasheet
#define IR_SLAVE_ADDR 0x5A
#define AMB_POLL_ADDR 0x06
#define OBJ_POLL_ADDR 0x07

// buffer sizing defines
#define IR_POLL_BFR_SIZE 3u
#define CAN_TRANSMIT_TICKS    100

#define SETTLE_TICKS          200
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

// OS thread definitions
osThreadId_t thermalTaskHandle;
const osThreadAttr_t thermalTask_attributes = {
  .name = "thermalTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

// Sensor Read Structs
typedef struct __attribute__((packed)) {
  uint8_t lsb;  // i2c reads for IR sensor come in 2 bytes
  uint8_t msb;
  uint8_t crc;  // checksum for valid data
} thermo_read_raw_t;

// Single Variables
volatile thermo_read_raw_t obj_read_raw = {0};
volatile thermo_read_raw_t amb_read_raw = {0};

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for CANTransmitTask */
osThreadId_t CANTransmitTaskHandle;
uint32_t CANTransmitTaskBuffer[ 128 ];
osStaticThreadDef_t CANTransmitTaskControlBlock;
const osThreadAttr_t CANTransmitTask_attributes = {
  .name = "CANTransmitTask",
  .cb_mem = &CANTransmitTaskControlBlock,
  .cb_size = sizeof(CANTransmitTaskControlBlock),
  .stack_mem = &CANTransmitTaskBuffer[0],
  .stack_size = sizeof(CANTransmitTaskBuffer),
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for DisplayTask */
osThreadId_t DisplayTaskHandle;
const osThreadAttr_t DisplayTask_attributes = {
  .name = "DisplayTask",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};
/* Definitions for FIFOCANTransmit */
osMessageQueueId_t FIFOCANTransmitHandle;
uint8_t FIFOCANTransmitBuffer[ 16 * sizeof( uint16_t ) ];
osStaticMessageQDef_t FIFOCANTransmitControlBlock;
const osMessageQueueAttr_t FIFOCANTransmit_attributes = {
  .name = "FIFOCANTransmit",
  .cb_mem = &FIFOCANTransmitControlBlock,
  .cb_size = sizeof(FIFOCANTransmitControlBlock),
  .mq_mem = &FIFOCANTransmitBuffer,
  .mq_size = sizeof(FIFOCANTransmitBuffer)
};
/* Definitions for EnableCANTx */
osTimerId_t EnableCANTxHandle;
const osTimerAttr_t EnableCANTx_attributes = {
  .name = "EnableCANTx"
};
/* Definitions for CANTxEvent */
osEventFlagsId_t CANTxEventHandle;
osStaticEventGroupDef_t EnableCANTxControlBlock;
const osEventFlagsAttr_t CANTxEvent_attributes = {
  .name = "CANTxEvent",
  .cb_mem = &EnableCANTxControlBlock,
  .cb_size = sizeof(EnableCANTxControlBlock),
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void StartThermalSensorTask(void *argument);

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void CANTransmit(void *argument);
void refreshDisplay(void *argument);
void EnableCANTxCallback(void *argument);

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
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* Create the timer(s) */
  /* creation of EnableCANTx */
  EnableCANTxHandle = osTimerNew(EnableCANTxCallback, osTimerPeriodic, NULL, &EnableCANTx_attributes);

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  osTimerStart(EnableCANTxHandle, CAN_TRANSMIT_TICKS);
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of FIFOCANTransmit */
  FIFOCANTransmitHandle = osMessageQueueNew (16, sizeof(uint16_t), &FIFOCANTransmit_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of CANTransmitTask */
  CANTransmitTaskHandle = osThreadNew(CANTransmit, NULL, &CANTransmitTask_attributes);

  /* creation of DisplayTask */
  DisplayTaskHandle = osThreadNew(refreshDisplay, NULL, &DisplayTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  thermalTaskHandle = osThreadNew(StartThermalSensorTask, NULL, &thermalTask_attributes);
  /* USER CODE END RTOS_THREADS */

  /* Create the event(s) */
  /* creation of CANTxEvent */
  CANTxEventHandle = osEventFlagsNew(&CANTxEvent_attributes);

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

/* USER CODE BEGIN Header_CANTransmit */
/**
* @brief Function implementing the CANTransmitTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_CANTransmit */
void CANTransmit(void *argument)
{
  /* USER CODE BEGIN CANTransmit */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1000);
  }
  /* USER CODE END CANTransmit */
}

/* USER CODE BEGIN Header_refreshDisplay */
/**
* @brief Function implementing the DisplayTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_refreshDisplay */
void refreshDisplay(void *argument)
{
  /* USER CODE BEGIN refreshDisplay */
  // Init Display
  HAL_GPIO_WritePin(LCD_BLK_GPIO_Port, LCD_BLK_Pin, GPIO_PIN_RESET);
  ST7735_Init();
  ST7735_FillScreen(ST7735_BLACK);

  /* Infinite loop */
  for(;;)
  {
    // Sleep to yield CPU
    osDelay(100);
  }
  /* USER CODE END refreshDisplay */
}

/* EnableCANTxCallback function */
void EnableCANTxCallback(void *argument)
{
  /* USER CODE BEGIN EnableCANTxCallback */

  /* USER CODE END EnableCANTxCallback */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

extern I2C_HandleTypeDef hi2c1; // Assuming main I2C refers to hi2c1

void StartThermalSensorTask(void *argument) {
  for(;;) {
    HAL_I2C_Mem_Read_DMA(&hi2c1, (IR_SLAVE_ADDR << 1), OBJ_POLL_ADDR, I2C_MEMADD_SIZE_8BIT, (uint8_t*)&obj_read_raw, IR_POLL_BFR_SIZE);
    HAL_I2C_Mem_Read_DMA(&hi2c1, (IR_SLAVE_ADDR << 1), AMB_POLL_ADDR, I2C_MEMADD_SIZE_8BIT, (uint8_t*)&amb_read_raw, IR_POLL_BFR_SIZE);

    // OBJ Handling
    if(MLX90614_VerifyData(OBJ_POLL_ADDR, (volatile uint8_t *)&obj_read_raw)) {
      //CRC Check Passed -- Proceed with Data Processing
      uint16_t obj_read_val = obj_read_raw.msb << 8 | obj_read_raw.lsb;
    }

    // AMB Handling
    if(MLX90614_VerifyData(AMB_POLL_ADDR, (volatile uint8_t *)&amb_read_raw)) {
      //CRC Check Passed -- Proceed with Data Processing
      uint16_t amb_read_val = amb_read_raw.msb << 8 | amb_read_raw.lsb;
    }

    osDelay(20);
  }
}

/* USER CODE END Application */

