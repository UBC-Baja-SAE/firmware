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
#include <string.h>
#include <stdbool.h>
#include "FreeRTOSConfig.h"
#include "SEGGER_RTT.h"
#include "st7735.h"
#include "roxy.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct {
  uint32_t id;
  uint8_t  len;
  uint8_t  data[64];
} CanFrame_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
static osMessageQueueId_t CanTxQueueHandle;
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for CANTransmitTask */
osThreadId_t CANTransmitTaskHandle;
const osThreadAttr_t CANTransmitTask_attributes = {
  .name = "CANTransmitTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for DisplayTask */
osThreadId_t DisplayTaskHandle;
const osThreadAttr_t DisplayTask_attributes = {
  .name = "DisplayTask",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};
/* Definitions for exampleTask */
osThreadId_t exampleTaskHandle;
const osThreadAttr_t exampleTask_attributes = {
  .name = "exampleTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
static uint32_t len_to_dlc(uint8_t n);
static void CAN_Tx(uint32_t id, const uint8_t *data, uint8_t len);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void CANTransmit(void *argument);
void RefreshDisplay(void *argument);
void exampleRead(void *argument);

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

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  CanTxQueueHandle = osMessageQueueNew(16, sizeof(CanFrame_t), NULL);
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of CANTransmitTask */
  CANTransmitTaskHandle = osThreadNew(CANTransmit, NULL, &CANTransmitTask_attributes);

  /* creation of DisplayTask */
  DisplayTaskHandle = osThreadNew(RefreshDisplay, NULL, &DisplayTask_attributes);

  /* creation of exampleTask */
  exampleTaskHandle = osThreadNew(exampleRead, NULL, &exampleTask_attributes);

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


  // SystemView Start
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
  extern FDCAN_HandleTypeDef hfdcan1;
  CanFrame_t f;

  FDCAN_TxHeaderTypeDef hdr = {
    .IdType              = FDCAN_STANDARD_ID,
    .TxFrameType         = FDCAN_DATA_FRAME,
    .ErrorStateIndicator = FDCAN_ESI_ACTIVE,
    .BitRateSwitch       = FDCAN_BRS_OFF,
    .FDFormat            = FDCAN_FD_CAN,
    .TxEventFifoControl  = FDCAN_NO_TX_EVENTS,
    .MessageMarker       = 0
  };

  HAL_FDCAN_Start(&hfdcan1);

  for (;;)
  {
    if (osMessageQueueGet(CanTxQueueHandle, &f, NULL, osWaitForever) == osOK) {
      hdr.Identifier = f.id;
      hdr.DataLength = len_to_dlc(f.len);

      while (HAL_FDCAN_GetTxFifoFreeLevel(&hfdcan1) == 0) osDelay(1);

      // TODO: Handle bus-off recovery, (enable auto retransmission once complete) and add fdcan fifo queue timeouts incase queue is full

      HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &hdr, f.data);
    }
  }
  /* USER CODE END CANTransmit */
}

/* USER CODE BEGIN Header_RefreshDisplay */
/**
* @brief Function implementing the DisplayTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_RefreshDisplay */
void RefreshDisplay(void *argument)
{
  /* USER CODE BEGIN RefreshDisplay */
  // Init Display
  HAL_GPIO_WritePin(LCD_BLK_GPIO_Port, LCD_BLK_Pin, GPIO_PIN_RESET);
  ST7735_Init();
  ST7735_FillScreen(ST7735_GREEN);

  /* Infinite loop */
  for(;;)
  {
    // Sleep to yield CPU
    osDelay(100);
  }
  /* USER CODE END RefreshDisplay */
}

/* USER CODE BEGIN Header_exampleRead */
/**
* @brief Function implementing the exampleTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_exampleRead */
void exampleRead(void *argument)
{
  /* USER CODE BEGIN exampleRead */

  // DEADBEEF hextype sample message
  uint8_t test_12[12] = {0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF};

  /* Infinite loop */
  for(;;)
  {
    // Transmits every second
    CAN_Tx(0x001, test_12, 12);
    osDelay(1000);
  }
  /* USER CODE END exampleRead */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

// Dynamically converts message lengths to CAN frame formats
static uint32_t len_to_dlc(uint8_t n)
{
  if (n <= 8)  return ((uint32_t)n << 16);
  if (n <= 12) return FDCAN_DLC_BYTES_12;
  if (n <= 16) return FDCAN_DLC_BYTES_16;
  if (n <= 20) return FDCAN_DLC_BYTES_20;
  if (n <= 24) return FDCAN_DLC_BYTES_24;
  if (n <= 32) return FDCAN_DLC_BYTES_32;
  if (n <= 48) return FDCAN_DLC_BYTES_48;
  return FDCAN_DLC_BYTES_64;
}

// Adds message to FreeRTOS Queue
static void CAN_Tx(uint32_t id, const uint8_t *data, uint8_t len)
{
  if (len > 64) len = 64;
  CanFrame_t f = { .id = id, .len = len };
  memcpy(f.data, data, len);

  (void)osMessageQueuePut(CanTxQueueHandle, &f, 0, 0);
}
/* USER CODE END Application */

