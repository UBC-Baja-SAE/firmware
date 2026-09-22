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
#include <stdint.h>
#include "roxy.h"
#include "helpers.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
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

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

// OS thread definitions
osThreadId_t thermalTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
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
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

void StartThermalSensorTask(void *argument);

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

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

StartThermalSensorTask(void *argument) {
  HAL_I2C_Mem_Read_DMA(&hi2c1, (IR_SLAVE_ADDR << 1), OBJ_POLL_ADDR, I2C_MEMADD_SIZE_8BIT, &obj_read_raw, IR_POLL_BFR_SIZE);
  HAL_I2C_Mem_Read_DMA(&hi2c1, (IR_SLAVE_ADDR << 1), AMB_POLL_ADDR, I2C_MEMADD_SIZE_8BIT, &amb_read_raw, IR_POLL_BFR_SIZE);

  // OBJ Handling
  if(MLX90614_VerifyData(OBJ_POLL_ADDR, (volatile uint8_t *)&obj_read_raw)) {

    //CRC Check Passed -- Proceed with Data Processing
    uint16_t obj_read_val = obj_read_raw.msb << 8 | obj_read_raw.lsb;

    m
  }
  else {
    // CRC Check Failed -- Do Nothing
  }

  // AMB Handling
  if(MLX90614_VerifyData(AMB_POLL_ADDR, (volatile uint8_t *)&amb_read_raw)) {

    //CRC Check Passed -- Proceed with Data Processing
    uint16_t amb_read_val = amb_read_raw.msb << 8 | amb_read_raw.lsb;


  }
  else {
    // CRC Check Failed -- Do Nothing
  }

  osDelay(20);
}

/* USER CODE END Application */

