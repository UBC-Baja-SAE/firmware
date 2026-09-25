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
#include "tim.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef StaticTask_t osStaticThreadDef_t;
typedef StaticQueue_t osStaticMessageQDef_t;
typedef StaticEventGroup_t osStaticEventGroupDef_t;
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define TACH_TX_READY        0x01
#define SPEEDO_TX_READY      0x02
#define IMU_TX_READY         0x04
#define ALL_SENSORS_TX_READY (TACH_TX_READY | SPEEDO_TX_READY | IMU_TX_READY)


#define SPEED_SMOOTHING         0.15f
#define TACH_SMOOTHING          0.10f
#define WHEEL_CIRCUMFERENCE_M   1.675f
#define SPEEDO_PULSES_PER_REV   1.0f
#define TACH_PULSES_PER_REV     1.0f
#define MAX_SPEED_KMH           99U
#define MAX_RPM_LIMIT           9999U
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
volatile uint32_t current_speed = 0;
volatile uint32_t current_tach = 0;

static volatile float smoothed_speed_freq = 0.0f;
static volatile float smoothed_tach_freq = 0.0f;

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 1024 * 4,
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

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void SpeedoRead(void *argument);
void CANTransmit(void *argument);
void TachRead(void *argument);
void IMURead(void *argument);
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
  osTimerStart(EnableCANTxHandle, 100);
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

  /* creation of DisplayTask */
  DisplayTaskHandle = osThreadNew(refreshDisplay, NULL, &DisplayTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
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
  osDelay(200);

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
  HAL_TIM_Base_Start(&htim5);

  uint32_t prev_tick = osKernelGetTickCount();
  uint32_t prev_speedo_pulse_count = 0;

  /* Infinite loop */
  for(;;)
  {
    osEventFlagsWait(CANTxEventHandle, SPEEDO_TX_READY, osFlagsWaitAny, osWaitForever);

    uint32_t current_tick = osKernelGetTickCount();
    uint32_t speedo_pulse_count = __HAL_TIM_GET_COUNTER(&htim5);

    uint32_t tick_delta = current_tick - prev_tick;
    uint32_t speedo_interval_total = speedo_pulse_count - prev_speedo_pulse_count;

    prev_tick = current_tick;
    prev_speedo_pulse_count = speedo_pulse_count;

    float elapsed_seconds = (float)tick_delta / 1000.0f;

    if (elapsed_seconds > 0.0f) {
      // Calculate raw Hz
      float raw_hz = (float)speedo_interval_total / elapsed_seconds;

      // Check if pulses are 0
      if (speedo_interval_total == 0) {
        raw_hz = 0.0f;
      }

      // Apply Exponential Moving Average smoothing
      smoothed_speed_freq = (raw_hz * SPEED_SMOOTHING) + (smoothed_speed_freq * (1.0f - SPEED_SMOOTHING));

      // Convert Hz to m/s, then km/h
      float speed_ms = (smoothed_speed_freq / SPEEDO_PULSES_PER_REV) * WHEEL_CIRCUMFERENCE_M;
      uint32_t kmh = (uint32_t)(speed_ms * 3.6f);

      // Apply limit
      current_speed = (kmh > MAX_SPEED_KMH) ? MAX_SPEED_KMH : kmh;
    }

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
    osDelay(1000);
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
  HAL_TIM_Base_Start(&htim2);

  uint32_t prev_tick = osKernelGetTickCount();
  uint32_t prev_tach_pulse_count = 0;

  /* Infinite loop */
  for(;;)
  {
    osEventFlagsWait(CANTxEventHandle,TACH_TX_READY,osFlagsWaitAny,osWaitForever);

    uint32_t current_tick = osKernelGetTickCount();
    uint32_t tach_pulse_count = __HAL_TIM_GET_COUNTER(&htim2);

    uint32_t tick_delta = current_tick - prev_tick;
    uint32_t tach_interval_total = tach_pulse_count - prev_tach_pulse_count;

    prev_tick = current_tick;
    prev_tach_pulse_count = tach_pulse_count;

    float elapsed_seconds = (float)tick_delta / 1000.0f;

    if (elapsed_seconds > 0.0f) {
      // Calculate raw Hz
      float raw_hz = (float)tach_interval_total / elapsed_seconds;

      // Check if pulses are 0
      if (tach_interval_total == 0) {
        raw_hz = 0.0f;
      }

      // Apply Exponential Moving Average smoothing
      smoothed_tach_freq = (raw_hz * TACH_SMOOTHING) + (smoothed_tach_freq * (1.0f - TACH_SMOOTHING));

      // Convert Hz to RPM using your new #define
      uint32_t calculated_rpm = (uint32_t)((smoothed_tach_freq * 60.0f) / TACH_PULSES_PER_REV);

      // Apply limit
      current_tach = (calculated_rpm > MAX_RPM_LIMIT) ? MAX_RPM_LIMIT : calculated_rpm;
    }
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
    osDelay(1000);
  }
  /* USER CODE END IMURead */
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
  char buf_speed[16];
  char buf_tach[16];

  // Init Display
  HAL_GPIO_WritePin(LCD_BLK_GPIO_Port, LCD_BLK_Pin, GPIO_PIN_RESET);
  ST7735_Init();
  ST7735_FillScreen(ST7735_BLACK);

  /* Infinite loop */
  for(;;)
  {
    // Speedometer
    snprintf(buf_speed, sizeof(buf_speed), "%2lu km/h", current_speed);
    ST7735_WriteString(70, 20, buf_speed, Font_11x18, ST7735_WHITE, ST7735_BLACK);

    // Tachometer
    snprintf(buf_tach, sizeof(buf_tach), "%4lu rpm", current_tach);
    ST7735_WriteString(48, 50, buf_tach, Font_11x18, ST7735_WHITE, ST7735_BLACK);

    // Sleep to yield CPU
    osDelay(100);
  }
  /* USER CODE END refreshDisplay */
}

/* EnableCANTxCallback function */
void EnableCANTxCallback(void *argument)
{
  /* USER CODE BEGIN EnableCANTxCallback */

  osEventFlagsSet(CANTxEventHandle, ALL_SENSORS_TX_READY);

  /* USER CODE END EnableCANTxCallback */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

