/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include <stdlib.h>
#include "fdcan.h"
#include "mochi.h"
#include "i2c.h"
#include "semphr.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define ICM42670_I2C_ADDR (0x68 << 1) // 0xD0
#define REG_ACCEL_DATA_X1 0x0B
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
// Fast median filter for 3 values
static inline int16_t median3(int16_t a, int16_t b, int16_t c) {
  if (a > b) {
    if (b > c) return b;
    else if (a > c) return c;
    else return a;
  } else {
    if (a > c) return a;
    else if (b > c) return c;
    else return b;
  }
}
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
extern volatile uint32_t speedometer_kmh;
extern volatile uint32_t tachometer_rpm;
extern void Check_Sensor_Timeouts(void);

SemaphoreHandle_t IMU_DataReady_Sem;
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for ReadIMU */
osThreadId_t ReadIMUHandle;
const osThreadAttr_t ReadIMU_attributes = {
  .name = "ReadIMU",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void ReadIMUTask(void *argument);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void ReadIMUTask(void *argument);

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
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  IMU_DataReady_Sem = xSemaphoreCreateBinary();
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of ReadIMU */
  ReadIMUHandle = osThreadNew(ReadIMUTask, NULL, &ReadIMU_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
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

    /* 1. Transmit Speedometer Message */
    TxHeader.Identifier = MOCHI_SPEEDOMETER_FRAME_ID;
    TxHeader.DataLength = FDCAN_DLC_BYTES_2;

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

    /* 2. Transmit Tachometer Message */
    TxHeader.Identifier = MOCHI_TACHOMETER_FRAME_ID;
    TxHeader.DataLength = FDCAN_DLC_BYTES_2;

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

/* USER CODE BEGIN Header_ReadIMUTask */
/**
* @brief Function implementing the ReadIMU thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_ReadIMUTask */
void ReadIMUTask(void *argument)
{
  /* USER CODE BEGIN ReadIMUTask */
  if (IMU_Init() != HAL_OK)
  {
    vTaskSuspend(NULL);
  }

  uint8_t rawData[12];
  uint8_t TxData[8];
  struct mochi_rear_accel_t accel_msg;
  struct mochi_rear_gyro_t gyro_msg;

  FDCAN_TxHeaderTypeDef TxHeader;
  TxHeader.IdType = FDCAN_STANDARD_ID;
  TxHeader.TxFrameType = FDCAN_DATA_FRAME;
  TxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
  TxHeader.BitRateSwitch = FDCAN_BRS_OFF;
  TxHeader.FDFormat = FDCAN_CLASSIC_CAN;
  TxHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
  TxHeader.MessageMarker = 0;
  TxHeader.DataLength = FDCAN_DLC_BYTES_6;

  // History buffers for the 3-sample sliding window
  int16_t ax_hist[3] = {0}, ay_hist[3] = {0}, az_hist[3] = {0};
  int16_t gx_hist[3] = {0}, gy_hist[3] = {0}, gz_hist[3] = {0};
  uint8_t hist_idx = 0;

  /* Infinite loop */
  for(;;)
  {
    // Wait for the EXTI interrupt
    if (xSemaphoreTake(IMU_DataReady_Sem, portMAX_DELAY) == pdTRUE)
    {
      if (HAL_I2C_Mem_Read(&hi2c1, ICM42670_I2C_ADDR, REG_ACCEL_DATA_X1,
                                 I2C_MEMADD_SIZE_8BIT, rawData, 12, 100) == HAL_OK)
      {
        ax_hist[hist_idx] = (int16_t)((rawData[0] << 8) | rawData[1]);
        ay_hist[hist_idx] = (int16_t)((rawData[2] << 8) | rawData[3]);
        az_hist[hist_idx] = (int16_t)((rawData[4] << 8) | rawData[5]);

        gx_hist[hist_idx] = (int16_t)((rawData[6] << 8)  | rawData[7]);
        gy_hist[hist_idx] = (int16_t)((rawData[8] << 8)  | rawData[9]);
        gz_hist[hist_idx] = (int16_t)((rawData[10] << 8) | rawData[11]);

        hist_idx = (hist_idx + 1) % 3;

        /* Filter & Transmit Accelerometer */
        accel_msg.accel_x = median3(ax_hist[0], ax_hist[1], ax_hist[2]);
        accel_msg.accel_y = median3(ay_hist[0], ay_hist[1], ay_hist[2]);
        accel_msg.accel_z = median3(az_hist[0], az_hist[1], az_hist[2]);

        mochi_rear_accel_pack(TxData, &accel_msg, 6);
        TxHeader.Identifier = MOCHI_REAR_ACCEL_FRAME_ID;

        if (HAL_FDCAN_GetTxFifoFreeLevel(&hfdcan1) == 0) {
          uint32_t active_buffers = hfdcan1.Instance->TXBRP;
          if (active_buffers != 0) {
            HAL_FDCAN_AbortTxRequest(&hfdcan1, active_buffers);
            while ((hfdcan1.Instance->TXBRP & active_buffers) != 0);
          }
        }
        HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &TxHeader, TxData);

        /* Filter & Transmit Gyroscope */
        gyro_msg.gyro_x = median3(gx_hist[0], gx_hist[1], gx_hist[2]);
        gyro_msg.gyro_y = median3(gy_hist[0], gy_hist[1], gy_hist[2]);
        gyro_msg.gyro_z = median3(gz_hist[0], gz_hist[1], gz_hist[2]);

        mochi_rear_gyro_pack(TxData, &gyro_msg, 6);
        TxHeader.Identifier = MOCHI_REAR_GYRO_FRAME_ID;

        if (HAL_FDCAN_GetTxFifoFreeLevel(&hfdcan1) == 0) {
          uint32_t active_buffers = hfdcan1.Instance->TXBRP;
          if (active_buffers != 0) {
            HAL_FDCAN_AbortTxRequest(&hfdcan1, active_buffers);
            while ((hfdcan1.Instance->TXBRP & active_buffers) != 0);
          }
        }
        HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &TxHeader, TxData);
      }
      else
      {
        // AUTO-RECOVERY
        HAL_I2C_DeInit(&hi2c1);
        vTaskDelay(pdMS_TO_TICKS(5));
        MX_I2C1_Init();
      }
    }
  }
  /* USER CODE END ReadIMUTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
/**
* @brief Function implementing the ReadIMU thread.
*/

/* USER CODE END Application */

