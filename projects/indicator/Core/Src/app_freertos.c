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
#include "GC9A01.h"
#include "lvgl/lvgl.h"
#include "lv_port_disp.h"
#include "queue.h"
#include "roxy.h"

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
LV_FONT_DECLARE(microgramma);

static lv_obj_t * rpm_arc;
static lv_obj_t * rpm_label;

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 128 * 4
};
/* Definitions for lvgl */
osThreadId_t lvglHandle;
const osThreadAttr_t lvgl_attributes = {
  .name = "lvgl",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 1024 * 4
};
/* Definitions for can */
osThreadId_t canHandle;
const osThreadAttr_t can_attributes = {
  .name = "can",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 128 * 4
};
/* Definitions for lvglRead */
osMessageQueueId_t lvglReadHandle;
const osMessageQueueAttr_t lvglRead_attributes = {
  .name = "lvglRead"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

static void create_tachometer_ui(void)
{
  lv_obj_t * scr = lv_screen_active();
  lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);

  /* Thicker Outer Tachometer Arc */
  rpm_arc = lv_arc_create(scr);
  lv_obj_set_size(rpm_arc, 224, 224);
  lv_obj_center(rpm_arc);
  lv_arc_set_angles(rpm_arc, 135, 45);
  lv_arc_set_bg_angles(rpm_arc, 135, 45);
  lv_arc_set_range(rpm_arc, 0, 4000); // 0 to 4000 RPM
  lv_arc_set_value(rpm_arc, 0);

  lv_obj_remove_style(rpm_arc, NULL, LV_PART_KNOB);
  lv_obj_remove_flag(rpm_arc, LV_OBJ_FLAG_CLICKABLE);

  // Background track
  lv_obj_set_style_arc_width(rpm_arc, 24, LV_PART_MAIN);
  lv_obj_set_style_arc_color(rpm_arc, lv_color_hex(0x2A2A2A), LV_PART_MAIN);
  lv_obj_set_style_arc_rounded(rpm_arc, true, LV_PART_MAIN);

  // Yellow indicator
  lv_obj_set_style_arc_width(rpm_arc, 24, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(rpm_arc, lv_color_hex(0xFFD700), LV_PART_INDICATOR);
  lv_obj_set_style_arc_rounded(rpm_arc, true, LV_PART_INDICATOR);

  // Central Numeric Readout
  rpm_label = lv_label_create(scr);
  lv_label_set_text(rpm_label, "0");
  lv_obj_set_style_text_font(rpm_label, &lv_font_montserrat_48, LV_PART_MAIN);
  lv_obj_set_style_text_color(rpm_label, lv_color_white(), LV_PART_MAIN);
  lv_obj_align(rpm_label, LV_ALIGN_CENTER, 0, -10);

  // Subtitle Label
  lv_obj_t * unit_label = lv_label_create(scr);
  lv_label_set_text(unit_label, "RPM");
  lv_obj_set_style_text_font(unit_label, &lv_font_montserrat_18, LV_PART_MAIN);
  lv_obj_set_style_text_color(unit_label, lv_color_hex(0x758095), LV_PART_MAIN);
  lv_obj_align(unit_label, LV_ALIGN_CENTER, 0, 28);
}

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void lvglhandler(void *argument);
void canRead(void *argument);

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

  /* Create the queue(s) */
  /* creation of lvglRead */
  lvglReadHandle = osMessageQueueNew (4, sizeof(uint32_t), &lvglRead_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of lvgl */
  lvglHandle = osThreadNew(lvglhandler, NULL, &lvgl_attributes);

  /* creation of can */
  canHandle = osThreadNew(canRead, NULL, &can_attributes);

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

/* USER CODE BEGIN Header_lvglhandler */
/**
* @brief Function implementing the lvgl thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_lvglhandler */
void lvglhandler(void *argument)
{
  /* USER CODE BEGIN lvglhandler */
  GC9A01_Init();
  lv_init();
  lv_tick_set_cb(HAL_GetTick);
  lv_port_disp_init();

  create_tachometer_ui();

  uint32_t received_rpm = 0;

  /* Infinite loop */
  for(;;)
  {
    // Check queue for new CAN data
    if (osMessageQueueGet(lvglReadHandle, &received_rpm, NULL, 0) == osOK) {
      lv_arc_set_value(rpm_arc, received_rpm);

      lv_label_set_text_fmt(rpm_label, "%lu", (unsigned long)received_rpm);
    }

    uint32_t sleep_time = lv_timer_handler();
    if(sleep_time > 10) sleep_time = 10;
    if(sleep_time < 1)  sleep_time = 1;
    osDelay(sleep_time);
  }
  /* USER CODE END lvglhandler */
}

/* USER CODE BEGIN Header_canRead */
/**
* @brief Function implementing the can thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_canRead */
void canRead(void *argument)
{
  /* USER CODE BEGIN canRead */
  CAN_Rx_Frame_t received_frame;

  /* Infinite loop */
  for(;;) {
    // Block indefinitely until a raw CAN frame arrives
    if (xQueueReceive(can_rx_queue, &received_frame, portMAX_DELAY) == pdTRUE) {

      // Check if rear_daq
      if (received_frame.id == 200) {
        struct roxy_rear_daq_t rear_data;

        // Parse the raw payload bytes
        if (roxy_rear_daq_unpack(&rear_data, received_frame.payload, received_frame.length) == 0) {

          double rpm = roxy_rear_daq_tachometer_decode(rear_data.tachometer);

          // Cast to uint32_t and send to the LVGL task queue
          uint32_t rpm_to_send = (uint32_t)rpm;
          osMessageQueuePut(lvglReadHandle, &rpm_to_send, 0, 0);
        }
      }
    }
  }
  /* USER CODE END canRead */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

