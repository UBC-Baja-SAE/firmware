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

#define WHEEL_DIAMETER 0.55f
#define SPEED_SAMPLE_MS 1000U
#define SPEED_POLL_MS 1U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

volatile uint32_t pulseCountA4 = 0;
volatile uint32_t pulseCountA5 = 0;

volatile float speedA4 = 0.0f;
volatile float speedA5 = 0.0f;

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

osThreadId_t speedometerTaskHandle;

const osThreadAttr_t speedometerTask_attributes = {

  .name = "speedometerTask",
  .stack_size = 1024 * 4,
  .priority = osPriorityNormal

}

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

void speedometerTask(void *argument);

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

  speedometerTaskHandle = osThreadNew(

  speedometerTask, NULL, &speedometerTask_attributes

  );


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

void speedometerTask(void *argument) {

  GPIO_PinState previousA4 = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4);

  GPIO_PinState previousA5 = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5);

  uint32_t revolutionsA4 = 0;
  uint32_t revolutionsA5 = 0;

  const uint32_t tickFreq = osKernelGetTickFreq();
  const uint32_t sampleTicks = (SPEED_SAMPLE_MS * tickFreq + 999U) / 1000U;

  const uint32_t pollTicks = (SPEED_POLL_MS * tickFreq + 999U) / 1000U;

  uint32_t lastSample = osKernelGetTickCount();

  for (;;) {

    GPIO_PinState currentA4 =
        HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4);

    GPIO_PinState currentA5 =
        HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5);

    if (previousA4 == GPIO_PIN_RESET &&
        currentA4 == GPIO_PIN_SET)
    {
      revolutionsA4++;
      pulseCountA4++;
    }


    if (previousA5 == GPIO_PIN_RESET &&
        currentA5 == GPIO_PIN_SET)
    {
      revolutionsA5++;
      pulseCountA5++;
    }

    previousA4 = currentA4;
    previousA5 = currentA5;

    //check elapsed time

    uint32_t ticks = osKernelGetTickCount();
    uint32_t elapsedTicks = ticks - lastSample;

    if (elapsedTicks >= sampleTicks) {

      float elapsedSeconds = (float) elapsedTicks / (float) tickFreq;

      //convert revs to km/h

      speedA4 = (revolutionsA4 * 3.14159f * WHEEL_DIAMETER / (float) 5 * elapsedSeconds) * 3.6f;

      speedA5 = (revolutionsA5 * 3.14159f * WHEEL_DIAMETER / (float) 5 * elapsedSeconds) * 3.6f;

      //reset counters

      revolutionsA4 = 0;
      revolutionsA5 = 0;

      lastSample = ticks;

    }

    // Allow other FreeRTOS tasks to run
    osDelay(pollTicks > 0U ? pollTicks : 1U);

  }

}

/* USER CODE END Application */

