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
#include "can.h"
#include "mochi.h"
#include <stdbool.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
    STATE_IDLE,
    STATE_WAITING_ACK
} BtnState_t;

typedef struct {
    uint8_t id;
    uint8_t physical_state;
    uint8_t seq;
    BtnState_t state;
    uint32_t wait_start_time;
    bool trigger_send;
} ButtonTracker_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

// Import the TIM3 handle so we can read the raw hardware counter directly
extern TIM_HandleTypeDef htim3;

// Index 0: Button 1, Index 1: Button 2 (Calibration), Index 2: Toggle
volatile ButtonTracker_t btns[3] = {
    {1, 1, 0, STATE_IDLE, 0, false},
    {2, 1, 0, STATE_IDLE, 0, false},
    {3, 1, 0, STATE_IDLE, 0, false}
};

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

// Hardware Interrupt for CAN Reception (The Dash ACK)
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef rxHeader;
    uint8_t rxData[8];

    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rxHeader, rxData) == HAL_OK) {

        // 1280 (0x500) is the dash_ack message ID from the DBC
        if (rxHeader.StdId == 1280) {
            struct mochi_dash_ack_t ack;
            mochi_dash_ack_unpack(&ack, rxData, rxHeader.DLC);

            // Look for the matching button and sequence number
            for (int i = 0; i < 3; i++) {
                if (btns[i].id == ack.button_id &&
                    btns[i].state == STATE_WAITING_ACK &&
                    btns[i].seq == ack.seq) {

                    // ACK received! Transition back to IDLE
                    btns[i].state = STATE_IDLE;
                }
            }
        }
    }
}

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
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  CAN_TxHeaderTypeDef txHeader;
  uint32_t txMailbox;
  uint8_t txData[8];

  struct mochi_front_steering_t steering_msg;
  uint32_t steering_timer = 0;

  // Variable to store the steering offset on the STM32
  float steering_offset_deg = 0.0f;

  // Variables for software debouncing
  uint8_t btn_history[3] = {0, 0, 0};
  uint16_t btn_pins[3] = {Button_1_Pin, Button_2_Pin, Toggle_Pin};

  // --- CAN FILTER SETUP ---
  CAN_FilterTypeDef canFilterConfig;
  canFilterConfig.FilterBank = 0;
  canFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
  canFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
  canFilterConfig.FilterIdHigh = 0x0000;
  canFilterConfig.FilterIdLow = 0x0000;
  canFilterConfig.FilterMaskIdHigh = 0x0000;
  canFilterConfig.FilterMaskIdLow = 0x0000;
  canFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
  canFilterConfig.FilterActivation = ENABLE;
  canFilterConfig.SlaveStartFilterBank = 14;
  HAL_CAN_ConfigFilter(&hcan, &canFilterConfig);

  // Start the CAN Peripheral and enable RX Interrupts
  HAL_CAN_Start(&hcan);
  HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING);

  // Setup static header info for steering
  txHeader.StdId = 1538; // 0x602 front_steering
  txHeader.ExtId = 0;
  txHeader.IDE = CAN_ID_STD;
  txHeader.RTR = CAN_RTR_DATA;
  txHeader.DLC = 2;
  txHeader.TransmitGlobalTime = DISABLE;

  /* Infinite loop */
  for(;;)
  {
    uint32_t now = HAL_GetTick();

      // ----------------------------------------------------
      // 1. Shift-Register Software Debounce (Runs every 10ms)
      // ----------------------------------------------------
      for (int i = 0; i < 3; i++) {
          btn_history[i] = (btn_history[i] << 1) | (HAL_GPIO_ReadPin(GPIOB, btn_pins[i]) == GPIO_PIN_RESET ? 1 : 0);

          if (i < 2) {
              // FOR BUTTONS 1 & 2 (Momentary Push)
              // Trigger only on the moment of a solid press
              if ((btn_history[i] & 0x1F) == 0x0F) {
                  if (btns[i].state == STATE_IDLE) {
                      if (btns[i].id == 2) {
                          int16_t current_counter = (int16_t)__HAL_TIM_GET_COUNTER(&htim3);
                          steering_offset_deg = ((float)current_counter / 4.0f) * 11.25f;
                      }
                      btns[i].seq++;
                      btns[i].physical_state = 1; // Always 1 for momentary press
                      btns[i].trigger_send = true;
                      btns[i].state = STATE_WAITING_ACK;
                      btns[i].wait_start_time = now;
                  }
              }
          } else {
              // FOR TOGGLE SWITCH (Stateful)
              // Look for 5 consecutive readings of the exact same state to confirm a flip
              uint8_t stable_state = btn_history[i] & 0x1F;
              uint8_t new_state = btns[i].physical_state;

              if (stable_state == 0x1F) new_state = 1;      // Solidly ON
              else if (stable_state == 0x00) new_state = 0; // Solidly OFF

              // If the toggle flipped, trigger a CAN message
              if (new_state != btns[i].physical_state) {
                  btns[i].physical_state = new_state;
                  if (btns[i].state == STATE_IDLE) {
                      btns[i].seq++;
                      btns[i].trigger_send = true;
                      btns[i].state = STATE_WAITING_ACK;
                      btns[i].wait_start_time = now;
                  }
              }
          }
      }

    // ----------------------------------------------------
    // 2. Process Button State Machine (Send & Retry logic)
    // ----------------------------------------------------
    for (int i = 0; i < 3; i++) {
        bool timeout = (btns[i].state == STATE_WAITING_ACK) && (now - btns[i].wait_start_time > 500);

        if (btns[i].trigger_send || timeout) {
            btns[i].trigger_send = false;

            if (timeout) {
                btns[i].wait_start_time = now;
            }

            struct mochi_front_buttons_t btn_msg;
            btn_msg.button_id = btns[i].id;
            btn_msg.button_state = btns[i].physical_state;
            btn_msg.seq = btns[i].seq;

            CAN_TxHeaderTypeDef btnTxHeader;
            btnTxHeader.StdId = 1537; // 0x601 front_buttons
            btnTxHeader.ExtId = 0;
            btnTxHeader.IDE = CAN_ID_STD;
            btnTxHeader.RTR = CAN_RTR_DATA;
            btnTxHeader.DLC = 3;

            uint8_t txDataBtn[8];
            mochi_front_buttons_pack(txDataBtn, &btn_msg, btnTxHeader.DLC);

            if (HAL_CAN_GetTxMailboxesFreeLevel(&hcan) > 0) {
                HAL_CAN_AddTxMessage(&hcan, &btnTxHeader, txDataBtn, &txMailbox);
            }
        }
    }

    // ----------------------------------------------------
    // 3. Process Steering Angle (Send exactly every 100ms)
    // ----------------------------------------------------
    if (now - steering_timer >= 100) {
        steering_timer = now;

        // Read the hardware timer counter directly
        int16_t counter = (int16_t)__HAL_TIM_GET_COUNTER(&htim3);

        // Calculate the raw angle, forcing floating-point division
        float raw_angle = ((float)counter / 4.0f) * 11.25f;

        // Apply the offset before encoding
        float calibrated_angle = raw_angle - steering_offset_deg;

        steering_msg.steering_angle = mochi_front_steering_steering_angle_encode(calibrated_angle);
        mochi_front_steering_pack(txData, &steering_msg, txHeader.DLC);

        if (HAL_CAN_GetTxMailboxesFreeLevel(&hcan) > 0) {
            HAL_CAN_AddTxMessage(&hcan, &txHeader, txData, &txMailbox);
        }
    }

    osDelay(10);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */