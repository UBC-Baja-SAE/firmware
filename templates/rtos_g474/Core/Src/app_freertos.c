/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : app_freertos.c
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
#include <string.h>
#include <math.h>
#include <stdio.h>

#include "roxy.h"
#include "SEGGER_RTT.h"
#include "FreeRTOSConfig.h"
#include "fdcan.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef StaticTask_t osStaticThreadDef_t;
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
_Static_assert(configTICK_RATE_HZ == 1000, "CAN scheduler assumes a 1 kHz tick");
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
extern FDCAN_HandleTypeDef hfdcan2;

static struct roxy_rear_fast_t   g_fast;
static struct roxy_rear_medium_t g_medium;
static struct roxy_rear_slow_t   g_slow;

static FDCAN_TxHeaderTypeDef g_txHdr = {
  .IdType              = FDCAN_STANDARD_ID,
  .TxFrameType         = FDCAN_DATA_FRAME,
  .ErrorStateIndicator = FDCAN_ESI_ACTIVE,
  .BitRateSwitch       = FDCAN_BRS_ON,
  .FDFormat            = FDCAN_FD_CAN,
  .TxEventFifoControl  = FDCAN_NO_TX_EVENTS,
  .MessageMarker       = 0
};
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 1024 * 4
};
/* Definitions for CANTransmitTask */
osThreadId_t CANTransmitTaskHandle;
uint32_t CANTransmitTaskBuffer[ 1024 ];
osStaticThreadDef_t CANTransmitTaskControlBlock;
const osThreadAttr_t CANTransmitTask_attributes = {
  .name = "CANTransmitTask",
  .stack_mem = &CANTransmitTaskBuffer[0],
  .stack_size = sizeof(CANTransmitTaskBuffer),
  .cb_mem = &CANTransmitTaskControlBlock,
  .cb_size = sizeof(CANTransmitTaskControlBlock),
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for EnableCANTx */
osTimerId_t EnableCANTxHandle;
const osTimerAttr_t EnableCANTx_attributes = {
  .name = "EnableCANTx"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void SystemViewWait();
static void     Roxy_Copy(void *dst, const void *src, size_t n);
static void     CAN_Tx(uint32_t id, const uint8_t *buf, int len, int idx);
static uint32_t len_to_dlc(uint8_t n);

void Roxy_UpdateFast(const struct roxy_rear_fast_t *s);
void Roxy_UpdateSlow(const struct roxy_rear_slow_t *s);

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void CANTransmit(void *argument);
void EnableCANTxCallback(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

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
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of CANTransmitTask */
  CANTransmitTaskHandle = osThreadNew(CANTransmit, NULL, &CANTransmitTask_attributes);

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

  // SystemView TODO: (Comment this out if not using SystemView)
  SystemViewWait();

  /* Infinite loop */
  for(;;)
  {
    HAL_GPIO_TogglePin(GPIOC, LED_Pin);
    osDelay(1000);
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
  HAL_FDCAN_ConfigTxDelayCompensation(&hfdcan2,
          hfdcan2.Init.DataPrescaler * hfdcan2.Init.DataTimeSeg1, 0);
  HAL_FDCAN_EnableTxDelayCompensation(&hfdcan2);
  HAL_FDCAN_Start(&hfdcan2);

  uint8_t payload[64] = {0};
  struct roxy_rear_fast_t   fast;
  struct roxy_rear_medium_t medium;
  struct roxy_rear_slow_t   slow;
  uint32_t tick = osKernelGetTickCount();

  for (uint32_t slot = 0;; slot = (slot + 1) % 100)
  {
    osDelayUntil(++tick);

    Roxy_Copy(&fast, &g_fast, sizeof fast);
    CAN_Tx(ROXY_REAR_FAST_FRAME_ID, payload,
           roxy_rear_fast_pack(payload, &fast, sizeof payload), 0);

    if (slot % 10 == 0) {
      Roxy_Copy(&medium, &g_medium, sizeof medium);
      CAN_Tx(ROXY_REAR_MEDIUM_FRAME_ID, payload,
             roxy_rear_medium_pack(payload, &medium, sizeof payload), 1);
    }

    if (slot == 1) {
      Roxy_Copy(&slow, &g_slow, sizeof slow);
      CAN_Tx(ROXY_REAR_SLOW_FRAME_ID, payload,
             roxy_rear_slow_pack(payload, &slow, sizeof payload), 2);
    }
  }
  /* USER CODE END CANTransmit */
}

/* EnableCANTxCallback function */
void EnableCANTxCallback(void *argument)
{
  /* USER CODE BEGIN EnableCANTxCallback */

  /* USER CODE END EnableCANTxCallback */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void SystemViewWait() {
  osKernelLock();
  while (!SEGGER_RTT_HasData(1)) {}
  SEGGER_RTT_WriteNoLock(1, "SEGGER SystemView V3.52a\0\0\0\0\0\0\0", 32);
  for (uint8_t b = 0; SEGGER_RTT_ReadNoLock(1, &b, 1) != 1U || b != 1U;) {}
  SEGGER_SYSVIEW_Start();
  osKernelUnlock();
}

static void Roxy_Copy(void *dst, const void *src, size_t n)
{ taskENTER_CRITICAL(); memcpy(dst, src, n); taskEXIT_CRITICAL(); }

void Roxy_UpdateFast(const struct roxy_rear_fast_t *s)
{ Roxy_Copy(&g_fast, s, sizeof g_fast); }

void Roxy_UpdateSlow(const struct roxy_rear_slow_t *s)
{ Roxy_Copy(&g_slow, s, sizeof g_slow); }

static void CAN_Tx(uint32_t id, const uint8_t *buf, int len, int idx)
{
  (void)idx;
  if (len > 0) {
    g_txHdr.Identifier = id;
    g_txHdr.DataLength = len_to_dlc((uint8_t)len);
    if (HAL_FDCAN_GetTxFifoFreeLevel(&hfdcan2) > 0 &&
        HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan2, &g_txHdr, (uint8_t *)buf) == HAL_OK)
      return;
  }
}

static uint32_t len_to_dlc(uint8_t n)
{
  static const uint32_t small[9] = {
    FDCAN_DLC_BYTES_0, FDCAN_DLC_BYTES_1, FDCAN_DLC_BYTES_2,
    FDCAN_DLC_BYTES_3, FDCAN_DLC_BYTES_4, FDCAN_DLC_BYTES_5,
    FDCAN_DLC_BYTES_6, FDCAN_DLC_BYTES_7, FDCAN_DLC_BYTES_8
  };
  if (n <= 8)  return small[n];
  if (n <= 12) return FDCAN_DLC_BYTES_12;
  if (n <= 16) return FDCAN_DLC_BYTES_16;
  if (n <= 20) return FDCAN_DLC_BYTES_20;
  if (n <= 24) return FDCAN_DLC_BYTES_24;
  if (n <= 32) return FDCAN_DLC_BYTES_32;
  if (n <= 48) return FDCAN_DLC_BYTES_48;
  return FDCAN_DLC_BYTES_64;
}
/* USER CODE END Application */

