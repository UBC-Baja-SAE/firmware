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
#include <stdbool.h>

#include <std_msgs/msg/int32.h>
#include <std_msgs/msg/float32.h>

#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <uxr/client/transport.h>
#include <rmw_microxrcedds_c/config.h>
#include <rmw_microros/rmw_microros.h>

#include "fdcan.h"
#include "fdcan_transport.h"

#include <usart.h>
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
extern volatile uint32_t last_magnet_time;
extern volatile uint32_t tach_raw_value;


/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 3000 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
bool cubemx_transport_open(struct uxrCustomTransport * transport);
bool cubemx_transport_close(struct uxrCustomTransport * transport);
size_t cubemx_transport_write(struct uxrCustomTransport* transport, const uint8_t * buf, size_t len, uint8_t * err);
size_t cubemx_transport_read(struct uxrCustomTransport* transport, uint8_t* buf, size_t len, int timeout, uint8_t* err);

void * microros_allocate(size_t size, void * state);
void microros_deallocate(void * pointer, void * state);
void * microros_reallocate(void * pointer, size_t size, void * state);
void * microros_zero_allocate(size_t number_of_elements, size_t size_of_element, void * state);
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
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  rmw_uros_set_custom_transport(
    false,
    (void *)&hfdcan1,
    cubemx_transport_open,
    cubemx_transport_close,
    cubemx_transport_write,
    cubemx_transport_read
  );

  rcl_allocator_t freeRTOS_allocator = rcutils_get_zero_initialized_allocator();
  freeRTOS_allocator.allocate = microros_allocate;
  freeRTOS_allocator.deallocate = microros_deallocate;
  freeRTOS_allocator.reallocate = microros_reallocate;
  freeRTOS_allocator.zero_allocate = microros_zero_allocate;

  if (!rcutils_set_default_allocator(&freeRTOS_allocator)) {
      printf("Error on default allocators (line %d)\n", __LINE__);
  }

  rcl_publisher_t speed_publisher;
  rcl_publisher_t tach_publisher;
  std_msgs__msg__Int32 speed_msg;
  std_msgs__msg__Float32 tach_msg;
  rclc_support_t support;
  rcl_allocator_t allocator;
  rcl_node_t node;

  allocator = rcl_get_default_allocator();

  // Ping until agent responds
  while (rmw_uros_ping_agent(100, 5) != RMW_RET_OK) {
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
    osDelay(500);
  }

  // Solid on = connected
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);

  rclc_support_init(&support, 0, NULL, &allocator);
  rclc_node_init_default(&node, "rear_ecu", "", &support);

  rclc_publisher_init_default(
    &speed_publisher, &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
    "speedometer");

  rclc_publisher_init_default(
    &tach_publisher, &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float32),
    "tachometer");

  speed_msg.data = 0;
  tach_msg.data  = 0.0f;

  for (;;)
  {
    if (rmw_uros_ping_agent(50, 1) != RMW_RET_OK) {
      HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
      osDelay(500);
      continue;
    }

    speed_msg.data = (int32_t)speedometer_kmh;
    tach_msg.data  = 36.5f + ((float)tach_raw_value / 4095.0f) * 25.0f;

    rcl_publish(&speed_publisher, &speed_msg, NULL);
    rcl_publish(&tach_publisher,  &tach_msg,  NULL);

    osDelay(50);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
// void StartDefaultTask(void *argument)
// {
//   // Wait for FDCAN to be ready
//   osDelay(500);
//
//   // Try to send a raw frame completely bypassing micro-ROS
//   FDCAN_TxHeaderTypeDef txhdr = {
//     .Identifier          = 0x123,
//     .IdType              = FDCAN_STANDARD_ID,
//     .TxFrameType         = FDCAN_DATA_FRAME,
//     .DataLength          = FDCAN_DLC_BYTES_4,
//     .ErrorStateIndicator = FDCAN_ESI_ACTIVE,
//     .BitRateSwitch       = FDCAN_BRS_OFF,
//     .FDFormat            = FDCAN_CLASSIC_CAN,
//     .TxEventFifoControl  = FDCAN_NO_TX_EVENTS,
//     .MessageMarker       = 0,
// };
//   uint8_t data[4] = {0xDE, 0xAD, 0xBE, 0xEF};
//
//   // Start FDCAN here directly
//   HAL_StatusTypeDef start_ret = HAL_FDCAN_Start(&hfdcan1);
//   // start_ret should be HAL_OK (0)
//   // If HAL_ERROR (1), FDCAN peripheral isn't configured correctly
//
//   for (;;) {
//     HAL_StatusTypeDef tx_ret = HAL_FDCAN_AddMessageToTxFifoQ(
//         &hfdcan1, &txhdr, data);
//     // tx_ret should be HAL_OK
//     // If HAL_ERROR, Tx FIFO isn't configured (TxFifoQueueElmtsNbr = 0?)
//
//     HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
//     osDelay(200);
//   }
// }

/* USER CODE END Application */

