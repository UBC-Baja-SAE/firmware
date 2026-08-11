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
#include <stdlib.h>

#include "adc.h"
#include "semphr.h"
#include "fdcan.h"
#include "mochi.h"
#include "i2c.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct {
  uint32_t Identifier;
  uint8_t  Payload[8];
  uint32_t DataLength;
} CAN_Queue_Msg_t;
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

extern I2C_HandleTypeDef hi2c1;
extern FDCAN_HandleTypeDef hfdcan1;
extern ADC_HandleTypeDef hadc1;

QueueHandle_t CAN_Tx_Queue;
SemaphoreHandle_t IMU_DataReady_Sem;
SemaphoreHandle_t ADC_DataReady_Sem;

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
/* Definitions for CANTx */
osThreadId_t CANTxHandle;
const osThreadAttr_t CANTx_attributes = {
  .name = "CANTx",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for ReadLinPot */
osThreadId_t ReadLinPotHandle;
const osThreadAttr_t ReadLinPot_attributes = {
  .name = "ReadLinPot",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void ReadIMUTask(void *argument);
void CANTxTask(void *argument);
void ReadLinPotTask(void *argument);

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
  IMU_DataReady_Sem = xSemaphoreCreateBinary();
  ADC_DataReady_Sem = xSemaphoreCreateBinary();
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  CAN_Tx_Queue = xQueueCreate(32, sizeof(CAN_Queue_Msg_t));
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of ReadIMU */
  ReadIMUHandle = osThreadNew(ReadIMUTask, NULL, &ReadIMU_attributes);

  /* creation of CANTx */
  CANTxHandle = osThreadNew(CANTxTask, NULL, &CANTx_attributes);

  /* creation of ReadLinPot */
  ReadLinPotHandle = osThreadNew(ReadLinPotTask, NULL, &ReadLinPot_attributes);

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
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
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
  struct mochi_fr_accel_t accel_msg;
  struct mochi_fr_gyro_t gyro_msg;
  CAN_Queue_Msg_t txMsg;

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
        // Extract the raw bits directly into the history buffer
        ax_hist[hist_idx] = (int16_t)((rawData[0] << 8) | rawData[1]);
        ay_hist[hist_idx] = (int16_t)((rawData[2] << 8) | rawData[3]);
        az_hist[hist_idx] = (int16_t)((rawData[4] << 8) | rawData[5]);

        gx_hist[hist_idx] = (int16_t)((rawData[6] << 8)  | rawData[7]);
        gy_hist[hist_idx] = (int16_t)((rawData[8] << 8)  | rawData[9]);
        gz_hist[hist_idx] = (int16_t)((rawData[10] << 8) | rawData[11]);

        // Advance the circular buffer index (0, 1, 2, 0, 1...)
        hist_idx = (hist_idx + 1) % 3;

        // Filter Accelerometer
        memset(&txMsg, 0, sizeof(txMsg));
        accel_msg.accel_x = median3(ax_hist[0], ax_hist[1], ax_hist[2]);
        accel_msg.accel_y = median3(ay_hist[0], ay_hist[1], ay_hist[2]);
        accel_msg.accel_z = median3(az_hist[0], az_hist[1], az_hist[2]);

        mochi_fr_accel_pack(txMsg.Payload, &accel_msg, sizeof(txMsg.Payload));
        txMsg.Identifier = MOCHI_FR_ACCEL_FRAME_ID;
        txMsg.DataLength = 6;
        xQueueSend(CAN_Tx_Queue, &txMsg, pdMS_TO_TICKS(10));

        // Filter Gyroscope
        memset(&txMsg, 0, sizeof(txMsg));
        gyro_msg.gyro_x = median3(gx_hist[0], gx_hist[1], gx_hist[2]);
        gyro_msg.gyro_y = median3(gy_hist[0], gy_hist[1], gy_hist[2]);
        gyro_msg.gyro_z = median3(gz_hist[0], gz_hist[1], gz_hist[2]);

        mochi_fr_gyro_pack(txMsg.Payload, &gyro_msg, sizeof(txMsg.Payload));
        txMsg.Identifier = MOCHI_FR_GYRO_FRAME_ID;
        txMsg.DataLength = 6;
        xQueueSend(CAN_Tx_Queue, &txMsg, pdMS_TO_TICKS(10));
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

/* USER CODE BEGIN Header_CANTxTask */
/**
* @brief Function implementing the CANTx thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_CANTxTask */
void CANTxTask(void *argument)
{
  /* USER CODE BEGIN CANTxTask */
  CAN_Queue_Msg_t rxMsg;
  FDCAN_TxHeaderTypeDef TxHeader;

  // Initialize static FDCAN header parameters
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
    // Wait forever until a message is placed in the queue
    if (xQueueReceive(CAN_Tx_Queue, &rxMsg, portMAX_DELAY) == pdTRUE)
    {
      TxHeader.Identifier = rxMsg.Identifier;

      // Convert standard integer length to FDCAN bitfield macro
      const uint32_t FDCAN_DLC_Table[] = {
        FDCAN_DLC_BYTES_0, FDCAN_DLC_BYTES_1, FDCAN_DLC_BYTES_2, FDCAN_DLC_BYTES_3,
        FDCAN_DLC_BYTES_4, FDCAN_DLC_BYTES_5, FDCAN_DLC_BYTES_6, FDCAN_DLC_BYTES_7,
        FDCAN_DLC_BYTES_8
      };

      // Protect against out-of-bounds arrays if DataLength is > 8
      if (rxMsg.DataLength <= 8) {
        TxHeader.DataLength = FDCAN_DLC_Table[rxMsg.DataLength];
      } else {
        TxHeader.DataLength = FDCAN_DLC_BYTES_8;
      }

      uint8_t tx_timeout = 0;

      // Yield gracefully to other tasks if the hardware FIFO is temporarily full
      while (HAL_FDCAN_GetTxFifoFreeLevel(&hfdcan1) == 0)
      {
        vTaskDelay(pdMS_TO_TICKS(1));
        tx_timeout++;

        // If the bus is broken or disconnected, give up after 5ms
        if (tx_timeout >= 5)
        {
          break;
        }
      }

      // Only add the message to the FDCAN hardware queue if space actually freed up
      if (HAL_FDCAN_GetTxFifoFreeLevel(&hfdcan1) > 0)
      {
        HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &TxHeader, rxMsg.Payload);
      }
    }
  }
  /* USER CODE END CANTxTask */
}

/* USER CODE BEGIN Header_ReadLinPotTask */
/**
* @brief Function implementing the ReadLinPot thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_ReadLinPotTask */
void ReadLinPotTask(void *argument)
{
  /* USER CODE BEGIN ReadLinPotTask */
  CAN_Queue_Msg_t txMsg;
  uint32_t raw_adc_val;
  struct mochi_fr_linpot_t linpot_msg;

  /* Infinite loop */
  for(;;) {
    // Trigger the ADC conversion
    HAL_ADC_Start_IT(&hadc1);

    // Wait for the interrupt to say the conversion is done
    if (xSemaphoreTake(ADC_DataReady_Sem, portMAX_DELAY) == pdTRUE) {

      raw_adc_val = HAL_ADC_GetValue(&hadc1);

      // Clear the message structs before use
      memset(&txMsg, 0, sizeof(txMsg));
      mochi_fr_linpot_init(&linpot_msg); // Zeroes out the DBC struct

      // Calculate physical travel in cm
      // 36.5 base length + up to 25.0 cm of suspension stroke
      // Casted to double to match the cantools encode function signature
      double linpot_calc_cm = 36.5 + ((double)raw_adc_val / 4095.0) * 25.0;

      // Encode the physical value using the generated DBC function
      // This automatically applies the 0.01 scale factor from the DBC
      linpot_msg.travel = mochi_fr_linpot_travel_encode(linpot_calc_cm);

      // Pack the struct into the CAN payload array
      mochi_fr_linpot_pack(txMsg.Payload, &linpot_msg, sizeof(txMsg.Payload));

      // Set the CAN ID and DLC
      txMsg.Identifier = MOCHI_FR_LINPOT_FRAME_ID;
      txMsg.DataLength = 2;

      // Send it to the shared CAN transmission queue
      xQueueSend(CAN_Tx_Queue, &txMsg, pdMS_TO_TICKS(10));
    }

    // Wait
    vTaskDelay(pdMS_TO_TICKS(50));
  }
  /* USER CODE END ReadLinPotTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

