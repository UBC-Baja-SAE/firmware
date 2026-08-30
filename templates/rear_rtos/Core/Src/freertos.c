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
#include "SEGGER_RTT.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef StaticTask_t osStaticThreadDef_t;
typedef StaticQueue_t osStaticMessageQDef_t;
typedef StaticTimer_t osStaticTimerDef_t;
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

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for SpeedoReadTask */
osThreadId_t SpeedoReadTaskHandle;
uint32_t SpeedoReadBuffer[ 128 ];
osStaticThreadDef_t SpeedoReadControlBlock;
const osThreadAttr_t SpeedoReadTask_attributes = {
  .name = "SpeedoReadTask",
  .cb_mem = &SpeedoReadControlBlock,
  .cb_size = sizeof(SpeedoReadControlBlock),
  .stack_mem = &SpeedoReadBuffer[0],
  .stack_size = sizeof(SpeedoReadBuffer),
  .priority = (osPriority_t) osPriorityHigh,
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
/* Definitions for TachReadTask */
osThreadId_t TachReadTaskHandle;
uint32_t TachReadTaskBuffer[ 128 ];
osStaticThreadDef_t TachReadTaskControlBlock;
const osThreadAttr_t TachReadTask_attributes = {
  .name = "TachReadTask",
  .cb_mem = &TachReadTaskControlBlock,
  .cb_size = sizeof(TachReadTaskControlBlock),
  .stack_mem = &TachReadTaskBuffer[0],
  .stack_size = sizeof(TachReadTaskBuffer),
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for IMUReadTask */
osThreadId_t IMUReadTaskHandle;
uint32_t IMUReadTaskBuffer[ 128 ];
osStaticThreadDef_t IMUReadTaskControlBlock;
const osThreadAttr_t IMUReadTask_attributes = {
  .name = "IMUReadTask",
  .cb_mem = &IMUReadTaskControlBlock,
  .cb_size = sizeof(IMUReadTaskControlBlock),
  .stack_mem = &IMUReadTaskBuffer[0],
  .stack_size = sizeof(IMUReadTaskBuffer),
  .priority = (osPriority_t) osPriorityHigh,
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
/* Definitions for myTimer01 */
osTimerId_t myTimer01Handle;
osStaticTimerDef_t myTimer01ControlBlock;
const osTimerAttr_t myTimer01_attributes = {
  .name = "myTimer01",
  .cb_mem = &myTimer01ControlBlock,
  .cb_size = sizeof(myTimer01ControlBlock),
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void SpeedoRead(void *argument);
void CANTransmit(void *argument);
void TachRead(void *argument);
void IMURead(void *argument);
void Callback01(void *argument);

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
  /* creation of myTimer01 */
  myTimer01Handle = osTimerNew(Callback01, osTimerPeriodic, NULL, &myTimer01_attributes);

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
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

  /* creation of SpeedoReadTask */
  SpeedoReadTaskHandle = osThreadNew(SpeedoRead, NULL, &SpeedoReadTask_attributes);

  /* creation of CANTransmitTask */
  CANTransmitTaskHandle = osThreadNew(CANTransmit, NULL, &CANTransmitTask_attributes);

  /* creation of TachReadTask */
  TachReadTaskHandle = osThreadNew(TachRead, NULL, &TachReadTask_attributes);

  /* creation of IMUReadTask */
  IMUReadTaskHandle = osThreadNew(IMURead, NULL, &IMUReadTask_attributes);

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

  //SystemView Start
  osDelay(200);
  SEGGER_SYSVIEW_Start();

  /* Infinite loop */
  for(;;)
  {
    osDelay(100);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_SpeedoRead */
/**
* @brief Function implementing the SpeedoReadTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_SpeedoRead */
void SpeedoRead(void *argument)
{
  /* USER CODE BEGIN SpeedoRead */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END SpeedoRead */
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
    osDelay(1);
  }
  /* USER CODE END CANTransmit */
}

/* USER CODE BEGIN Header_TachRead */
/**
* @brief Function implementing the TachReadTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_TachRead */
void TachRead(void *argument)
{
  /* USER CODE BEGIN TachRead */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END TachRead */
}

/* USER CODE BEGIN Header_IMURead */
/**
* @brief Function implementing the IMUReadTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_IMURead */
void IMURead(void *argument)
{
  /* USER CODE BEGIN IMURead */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END IMURead */
}

/* Callback01 function */
void Callback01(void *argument)
{
  /* USER CODE BEGIN Callback01 */

  /* USER CODE END Callback01 */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

