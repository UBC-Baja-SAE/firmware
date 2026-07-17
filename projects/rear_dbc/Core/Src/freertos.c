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
#include "fdcan.h"
#include "mochi.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

extern volatile uint32_t speedometer_kmh;
extern volatile uint32_t tachometer_rpm;

extern void Check_Sensor_Timeouts(void);

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

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

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Gemini Slop
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */

  FDCAN_TxHeaderTypeDef TxHeader;
  uint8_t TxData[8] = {0};

  // Variables for the newly separated DBC messages
  struct mochi_speedometer_t speed_msg;
  struct mochi_tachometer_t tacho_msg;

  if (HAL_FDCAN_Start(&hfdcan1) != HAL_OK)
  {
    Error_Handler();
  }

  // Base configuration that applies to both messages
  TxHeader.IdType = FDCAN_STANDARD_ID;
  TxHeader.TxFrameType = FDCAN_DATA_FRAME;
  TxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
  TxHeader.BitRateSwitch = FDCAN_BRS_OFF;
  TxHeader.FDFormat = FDCAN_CLASSIC_CAN;
  TxHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
  TxHeader.MessageMarker = 0;

  /* Infinite loop */
  for(;;)
  {
    Check_Sensor_Timeouts();

    // Map extern vars to the structs
    speed_msg.speed = (uint16_t)speedometer_kmh;
    tacho_msg.rpm   = (uint16_t)tachometer_rpm;

    /* -------------------------------------------------------------
       1. Transmit Speedometer Message
       ------------------------------------------------------------- */
    TxHeader.Identifier = MOCHI_SPEEDOMETER_FRAME_ID;
    TxHeader.DataLength = FDCAN_DLC_BYTES_2; // Length changed from 4 to 2 based on DBC

    if (mochi_speedometer_pack(TxData, &speed_msg, MOCHI_SPEEDOMETER_LENGTH) > 0)
    {
      if (HAL_FDCAN_GetTxFifoFreeLevel(&hfdcan1) == 0)
      {
        uint32_t active_buffers = hfdcan1.Instance->TXBRP;
        if (active_buffers != 0)
        {
          HAL_FDCAN_AbortTxRequest(&hfdcan1, active_buffers);
          while ((hfdcan1.Instance->TXBRP & active_buffers) != 0);
        }
      }
      HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &TxHeader, TxData);
    }

    /* -------------------------------------------------------------
       2. Transmit Tachometer Message
       ------------------------------------------------------------- */
    TxHeader.Identifier = MOCHI_TACHOMETER_FRAME_ID;
    TxHeader.DataLength = FDCAN_DLC_BYTES_2; // Length changed from 4 to 2 based on DBC

    if (mochi_tachometer_pack(TxData, &tacho_msg, MOCHI_TACHOMETER_LENGTH) > 0)
    {
      if (HAL_FDCAN_GetTxFifoFreeLevel(&hfdcan1) == 0)
      {
        uint32_t active_buffers = hfdcan1.Instance->TXBRP;
        if (active_buffers != 0)
        {
          HAL_FDCAN_AbortTxRequest(&hfdcan1, active_buffers);
          while ((hfdcan1.Instance->TXBRP & active_buffers) != 0);
        }
      }
      HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &TxHeader, TxData);
    }

    osDelay(100);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

