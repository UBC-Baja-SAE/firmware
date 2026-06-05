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
#include <sensor_msgs/msg/imu.h>

#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <uxr/client/transport.h>
#include <rmw_microxrcedds_c/config.h>
#include <rmw_microros/rmw_microros.h>

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
extern volatile uint32_t linpot_raw_value;
extern volatile int16_t  imu_accel_x, imu_accel_y, imu_accel_z;
extern volatile int16_t  imu_gyro_x,  imu_gyro_y,  imu_gyro_z;


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

void ICM42670_Init(void);
void ICM42670_ReadData(void);
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
    true,
    (void *) &huart4,
    cubemx_transport_open,
    cubemx_transport_close,
    cubemx_transport_write,
    cubemx_transport_read);

  rcl_allocator_t freeRTOS_allocator = rcutils_get_zero_initialized_allocator();
  freeRTOS_allocator.allocate = microros_allocate;
  freeRTOS_allocator.deallocate = microros_deallocate;
  freeRTOS_allocator.reallocate = microros_reallocate;
  freeRTOS_allocator.zero_allocate = microros_zero_allocate;

  if (!rcutils_set_default_allocator(&freeRTOS_allocator)) {
    printf("Error on default allocators (line %d)\n", __LINE__);
  }

  rcl_publisher_t linpot_publisher;
  rcl_publisher_t imu_publisher;

  std_msgs__msg__Float32 linpot_msg;
  sensor_msgs__msg__Imu  imu_msg;

  rclc_support_t support;
  rcl_allocator_t allocator;
  rcl_node_t node;

  allocator = rcl_get_default_allocator();

  const int timeout_ms = 100;
  const uint8_t attempts = 5;

  while (rmw_uros_ping_agent(timeout_ms, attempts) != RMW_RET_OK) {
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_13);
    osDelay(500);
  }

  rmw_uros_sync_session(1000);

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
  osDelay(1000);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);

  rclc_support_init(&support, 0, NULL, &allocator);
  rclc_node_init_default(&node, "esus", "", &support);

  // Linear potentiometer message
  rclc_publisher_init_default(&linpot_publisher,  &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float32), "linpot");


  // Imu message
  rclc_publisher_init_default(&imu_publisher, &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, Imu), "imu/data_raw");

  memset(&imu_msg, 0, sizeof(sensor_msgs__msg__Imu));

  imu_msg.header.frame_id.data = (char*)"imu_link";
  imu_msg.header.frame_id.size = strlen(imu_msg.header.frame_id.data);
  imu_msg.header.frame_id.capacity = imu_msg.header.frame_id.size + 1;

  imu_msg.header.stamp.sec = 0;
  imu_msg.header.stamp.nanosec = 0;

  imu_msg.orientation.x = 0.0;
  imu_msg.orientation.y = 0.0;
  imu_msg.orientation.z = 0.0;
  imu_msg.orientation.w = 1.0;

  // No covariance data
  imu_msg.orientation_covariance[0] = -1.0;
  imu_msg.linear_acceleration_covariance[0] = -1.0;
  imu_msg.angular_velocity_covariance[0] = -1.0;

  const float GRAVITY_MSS = 9.80665f;
  const float DEG_TO_RAD  = 0.0174533f;

  const float ACCEL_SCALE = (1.0f / 4096.0f) * GRAVITY_MSS;
  const float GYRO_SCALE  = (1.0f / 16.4f)   * DEG_TO_RAD;

  for (;;)
  {

    if (rmw_uros_ping_agent(100, 3) != RMW_RET_OK) {
      NVIC_SystemReset();
    }

    ICM42670_ReadData();

    linpot_msg.data = 36.5f + ((float)linpot_raw_value / 4095.0f) * 25.0f;

    imu_msg.linear_acceleration.x = (float)imu_accel_x * ACCEL_SCALE;
    imu_msg.linear_acceleration.y = (float)imu_accel_y * ACCEL_SCALE;
    imu_msg.linear_acceleration.z = (float)imu_accel_z * ACCEL_SCALE;

    imu_msg.angular_velocity.x = (float)imu_gyro_x * GYRO_SCALE;
    imu_msg.angular_velocity.y = (float)imu_gyro_y * GYRO_SCALE;
    imu_msg.angular_velocity.z = (float)imu_gyro_z * GYRO_SCALE;


    //Timestamp sync every minute
    static uint32_t last_sync_time = 0;
    if (HAL_GetTick() - last_sync_time > 60000) {
      rmw_uros_sync_session(10);
      last_sync_time = HAL_GetTick();
    }

    int64_t time_ns = rmw_uros_epoch_nanos();

    imu_msg.header.stamp.sec = (int32_t)(time_ns / 1000000000);
    imu_msg.header.stamp.nanosec = (uint32_t)(time_ns % 1000000000);



    if (rcl_publish(&linpot_publisher, &linpot_msg, NULL) != RCL_RET_OK) {
      printf("Error publishing linpot (line %d)\n", __LINE__);
    }

    if (rcl_publish(&imu_publisher, &imu_msg, NULL) != RCL_RET_OK) {
      printf("Error publishing imu (line %d)\n", __LINE__);
    }

    osDelay(50);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

