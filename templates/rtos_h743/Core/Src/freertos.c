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
#include <string.h>
#include <math.h>
#include "FreeRTOSConfig.h"
#include "SEGGER_RTT.h"
#include "st7735.h"
#include "roxy.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
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
extern FDCAN_HandleTypeDef hfdcan1;

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
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for CANTransmitTask */
osThreadId_t CANTransmitTaskHandle;
const osThreadAttr_t CANTransmitTask_attributes = {
  .name = "CANTransmitTask",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for DisplayTask */
osThreadId_t DisplayTaskHandle;
const osThreadAttr_t DisplayTask_attributes = {
  .name = "DisplayTask",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for IMUTask */
osThreadId_t IMUTaskHandle;
const osThreadAttr_t IMUTask_attributes = {
  .name = "IMUTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for SpeedometerTask */
osThreadId_t SpeedometerTaskHandle;
const osThreadAttr_t SpeedometerTask_attributes = {
  .name = "SpeedometerTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};
/* Definitions for TachometerTask */
osThreadId_t TachometerTaskHandle;
const osThreadAttr_t TachometerTask_attributes = {
  .name = "TachometerTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};
/* Definitions for TemperatureTask */
osThreadId_t TemperatureTaskHandle;
const osThreadAttr_t TemperatureTask_attributes = {
  .name = "TemperatureTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
static void     Roxy_Copy(void *dst, const void *src, size_t n);
static void     CAN_Tx(uint32_t id, const uint8_t *buf, int len, int idx);
static uint32_t len_to_dlc(uint8_t n);
static float    wave(float t, float hz, float phase);
static void fake_imu(int16_t *ax, int16_t *ay, int16_t *az,
                     int16_t *gx, int16_t *gy, int16_t *gz, float t, float phase,
                     int imu_select);
void Roxy_UpdateFast(const struct roxy_rear_fast_t *s);
void Roxy_UpdateSlow(const struct roxy_rear_slow_t *s);
void Roxy_SetSpeedometer(uint16_t raw);
void Roxy_SetTachometer(uint16_t raw);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void CANTransmit(void *argument);
void RefreshDisplay(void *argument);
void IMURead(void *argument);
void SpeedometerRead(void *argument);
void TachometerRead(void *argument);
void TemperatureRead(void *argument);

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
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of CANTransmitTask */
  CANTransmitTaskHandle = osThreadNew(CANTransmit, NULL, &CANTransmitTask_attributes);

  /* creation of DisplayTask */
  DisplayTaskHandle = osThreadNew(RefreshDisplay, NULL, &DisplayTask_attributes);

  /* creation of IMUTask */
  IMUTaskHandle = osThreadNew(IMURead, NULL, &IMUTask_attributes);

  /* creation of SpeedometerTask */
  SpeedometerTaskHandle = osThreadNew(SpeedometerRead, NULL, &SpeedometerTask_attributes);

  /* creation of TachometerTask */
  TachometerTaskHandle = osThreadNew(TachometerRead, NULL, &TachometerTask_attributes);

  /* creation of TemperatureTask */
  TemperatureTaskHandle = osThreadNew(TemperatureRead, NULL, &TemperatureTask_attributes);

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
  HAL_FDCAN_ConfigTxDelayCompensation(&hfdcan1,
        hfdcan1.Init.DataPrescaler * hfdcan1.Init.DataTimeSeg1, 0);
  HAL_FDCAN_EnableTxDelayCompensation(&hfdcan1);
  HAL_FDCAN_Start(&hfdcan1);

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

/* USER CODE BEGIN Header_IMURead */
/**
* @brief Function implementing the IMUTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_IMURead */
void IMURead(void *argument)
{
  /* USER CODE BEGIN IMURead */
  struct roxy_rear_fast_t s = {0};
  uint32_t tick = osKernelGetTickCount();
  for (;;)
  {
    osDelayUntil(++tick);
    float t = (float)tick * 0.001f;

    fake_imu(&s.accel_x_l, &s.accel_y_l, &s.accel_z_l,
             &s.gyro_x_l,  &s.gyro_y_l,  &s.gyro_z_l, t, 0.0f, 0);
    fake_imu(&s.accel_x_r, &s.accel_y_r, &s.accel_z_r,
             &s.gyro_x_r,  &s.gyro_y_r,  &s.gyro_z_r, t, 1.0f, 1);
    fake_imu(&s.accel_x_c, &s.accel_y_c, &s.accel_z_c,
             &s.gyro_x_c,  &s.gyro_y_c,  &s.gyro_z_c, t, 2.0f, 2);

    Roxy_UpdateFast(&s);
  }
  /* USER CODE END IMURead */
}

/* USER CODE BEGIN Header_SpeedometerRead */
/**
* @brief Function implementing the SpeedometerTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_SpeedometerRead */
void SpeedometerRead(void *argument)
{
  /* USER CODE BEGIN SpeedometerRead */
  uint32_t tick = osKernelGetTickCount();
  for (;;)
  {
    tick += 10;
    osDelayUntil(tick);
    float kph = 20.0f + 15.0f * wave((float)tick * 0.001f, 0.1f, 0.0f);
    Roxy_SetSpeedometer(roxy_rear_medium_speedometer_encode(kph));
  }
  /* USER CODE END SpeedometerRead */
}

/* USER CODE BEGIN Header_TachometerRead */
/**
* @brief Function implementing the TachometerTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_TachometerRead */
void TachometerRead(void *argument)
{
  /* USER CODE BEGIN TachometerRead */
  uint32_t tick = osKernelGetTickCount();
  for (;;)
  {
    tick += 10;
    osDelayUntil(tick);
    float rpm = 2000.0f + 2000.0f * wave((float)tick * 0.001f, 0.2f, 0.0f);
    Roxy_SetTachometer(roxy_rear_medium_tachometer_encode(rpm));
  }
  /* USER CODE END TachometerRead */
}

/* USER CODE BEGIN Header_TemperatureRead */
/**
* @brief Function implementing the TemperatureTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_TemperatureRead */
void TemperatureRead(void *argument)
{
  /* USER CODE BEGIN TemperatureRead */
  struct roxy_rear_slow_t s = {0};
  uint32_t tick = osKernelGetTickCount();
  for (;;)
  {
    tick += 100;
    osDelayUntil(tick);
    float t = (float)tick * 0.001f;
    s.obj_temp = roxy_rear_slow_obj_temp_encode(70.0f + 50.0f * wave(t, 0.05f, 0.0f));
    s.amb_temp = roxy_rear_slow_amb_temp_encode(30.0f + 10.0f * wave(t, 0.03f, 0.0f));
    Roxy_UpdateSlow(&s);
  }
  /* USER CODE END TemperatureRead */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

static void Roxy_Copy(void *dst, const void *src, size_t n)
{ taskENTER_CRITICAL(); memcpy(dst, src, n); taskEXIT_CRITICAL(); }

void Roxy_UpdateFast(const struct roxy_rear_fast_t *s)
{ Roxy_Copy(&g_fast, s, sizeof g_fast); }

void Roxy_UpdateSlow(const struct roxy_rear_slow_t *s)
{ Roxy_Copy(&g_slow, s, sizeof g_slow); }

void Roxy_SetSpeedometer(uint16_t raw)
{ taskENTER_CRITICAL(); g_medium.speedometer = raw; taskEXIT_CRITICAL(); }

void Roxy_SetTachometer(uint16_t raw)
{ taskENTER_CRITICAL(); g_medium.tachometer = raw; taskEXIT_CRITICAL(); }

static void CAN_Tx(uint32_t id, const uint8_t *buf, int len, int idx)
{
  if (len > 0) {
    g_txHdr.Identifier = id;
    g_txHdr.DataLength = len_to_dlc((uint8_t)len);
    if (HAL_FDCAN_GetTxFifoFreeLevel(&hfdcan1) > 0 &&
        HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &g_txHdr, (uint8_t *)buf) == HAL_OK)
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

// Fake Data examples
static float wave(float t, float hz, float phase)
{ return sinf(6.2831853f * hz * t + phase); }

static void fake_imu(int16_t *ax, int16_t *ay, int16_t *az,
                     int16_t *gx, int16_t *gy, int16_t *gz, float t, float phase,
                     int imu_select) // 0=l, 1=r, 2=c
{
  if (imu_select == 0) {
    *ax = roxy_rear_fast_accel_x_l_encode(0.5f * wave(t, 1.0f, phase));
    *ay = roxy_rear_fast_accel_y_l_encode(0.5f * wave(t, 1.5f, phase));
    *az = roxy_rear_fast_accel_z_l_encode(1.0f + 0.2f * wave(t, 2.0f, phase));
    *gx = roxy_rear_fast_gyro_x_l_encode(100.0f * wave(t, 0.5f, phase));
    *gy = roxy_rear_fast_gyro_y_l_encode(100.0f * wave(t, 0.7f, phase));
    *gz = roxy_rear_fast_gyro_z_l_encode(100.0f * wave(t, 0.9f, phase));
  } else if (imu_select == 1) {
    *ax = roxy_rear_fast_accel_x_r_encode(0.5f * wave(t, 1.0f, phase));
    *ay = roxy_rear_fast_accel_y_r_encode(0.5f * wave(t, 1.5f, phase));
    *az = roxy_rear_fast_accel_z_r_encode(1.0f + 0.2f * wave(t, 2.0f, phase));
    *gx = roxy_rear_fast_gyro_x_r_encode(100.0f * wave(t, 0.5f, phase));
    *gy = roxy_rear_fast_gyro_y_r_encode(100.0f * wave(t, 0.7f, phase));
    *gz = roxy_rear_fast_gyro_z_r_encode(100.0f * wave(t, 0.9f, phase));
  } else {
    *ax = roxy_rear_fast_accel_x_c_encode(0.5f * wave(t, 1.0f, phase));
    *ay = roxy_rear_fast_accel_y_c_encode(0.5f * wave(t, 1.5f, phase));
    *az = roxy_rear_fast_accel_z_c_encode(1.0f + 0.2f * wave(t, 2.0f, phase));
    *gx = roxy_rear_fast_gyro_x_c_encode(100.0f * wave(t, 0.5f, phase));
    *gy = roxy_rear_fast_gyro_y_c_encode(100.0f * wave(t, 0.7f, phase));
    *gz = roxy_rear_fast_gyro_z_c_encode(100.0f * wave(t, 0.9f, phase));
  }
}
/* USER CODE END Application */

