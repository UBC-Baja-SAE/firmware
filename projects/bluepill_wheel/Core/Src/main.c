/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include "oled.h"

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
SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

//Suspension Mode
extern int Suspension_Mode;

// Push buttons (1-4)
uint32_t last_press_button1 = 0;
volatile uint8_t flag_button1 = 0;

uint32_t last_press_button2 = 0;
volatile uint8_t flag_button2 = 0;

uint32_t last_press_button3 = 0;
volatile uint8_t flag_button3 = 0;

uint32_t last_press_button4 = 0;
volatile uint8_t flag_button4 = 0;

// Toggle switches (1,2)
uint32_t last_press_toggle1 = 0;
volatile uint8_t flag_toggle1 = 0;

uint32_t last_press_toggle2 = 0;
volatile uint8_t flag_toggle2 = 0;

// Paddle buttons (1,2)
uint32_t last_press_paddle1 = 0;
volatile uint8_t flag_paddle1 = 0;

uint32_t last_press_paddle2 = 0;
volatile uint8_t flag_paddle2 = 0;

// Damping settings: Front Left, Front Right, Rear Left, Rear Right
int damping_FL = 0;
int damping_FR = 0;
int damping_RL = 0;
int damping_RR = 0;

// UART RX Variables for receiving damping settings
uint8_t rx_data;
char rx_buffer[32];
uint8_t rx_index = 0;
volatile uint8_t rx_flag = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_SPI1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void UART_send_byte(unsigned char data) {
  HAL_UART_Transmit(&huart1, &data, 1, HAL_MAX_DELAY);
}

void UART_send_message(const char* message) {
  for (size_t i = 0; i < strlen(message); ++i) {
    UART_send_byte((unsigned char)message[i]);
  }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  ssd1306_Init();
  ssd1306_Fill(Black);

  ssd1306_UpdateScreen();

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, RESET);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_15, RESET);

  drawBars();

  // Start UART Interrupt Reception for incoming damping settings
  HAL_UART_Receive_IT(&huart1, &rx_data, 1);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    // Check if UART data has arrived
    if (rx_flag) {
      UART_send_message("Received: [");
      UART_send_message(rx_buffer);
      UART_send_message("]\n");

      if (sscanf(rx_buffer, "%d,%d,%d,%d", &damping_FL, &damping_FR, &damping_RL, &damping_RR) == 4) {
        UART_send_message("Parse: SUCCESS\n");
        updateDampingDisplay(damping_FL, damping_FR, damping_RL, damping_RR);
        ssd1306_UpdateScreen();
      } else {
        UART_send_message("Parse: FAILED\n");
      }

      memset(rx_buffer, 0, sizeof(rx_buffer));
      rx_index = 0;
      rx_flag = 0;
    }

    if (flag_button1) {
      UART_send_message("B1\n");
      flag_button1 = 0;
      Suspension_Mode = 1;
      displaySuspensionMode(Suspension_Mode);
      updateDampingDisplay(damping_FL, damping_FR, damping_RL, damping_RR);
    }

    if (flag_button2) {
      UART_send_message("B2\n");
      flag_button2 = 0;
      Suspension_Mode = 2;
      displaySuspensionMode(Suspension_Mode);
      updateDampingDisplay(damping_FL, damping_FR, damping_RL, damping_RR);
    }

    if (flag_button3) {
      UART_send_message("B3\n");
      flag_button3 = 0;
      Suspension_Mode = 3;
      displaySuspensionMode(Suspension_Mode);
      updateDampingDisplay(damping_FL, damping_FR, damping_RL, damping_RR);
    }

    if (flag_button4) {
      UART_send_message("B4\n");
      flag_button4 = 0;
      Suspension_Mode = 4;
      displaySuspensionMode(Suspension_Mode);
      updateDampingDisplay(damping_FL, damping_FR, damping_RL, damping_RR);
    }

    if (flag_paddle1) {
      UART_send_message("P1\n");
      flag_paddle1 = 0;
    }

    if (flag_paddle2) {
      UART_send_message("P2\n");
      flag_paddle2 = 0;
    }

    if (flag_toggle1) {
      UART_send_message("T1\n");
      flag_toggle1 = 0;
      HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_15);
    }

    if (flag_toggle2) {
      UART_send_message("T2\n");
      flag_toggle2 = 0;
      HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_10);
    }

  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */
  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */
  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */
  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */
  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */
  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */
  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */
  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_15, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10|OLED_CS_Pin|OLED_DC_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(OLED_Reset_GPIO_Port, OLED_Reset_Pin, GPIO_PIN_SET);

  /*Configure GPIO pins : PC13 PC15 */
  GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PA0 PA1 PA2 PA3
                           PA4 PA5 PA6 PA7 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
                          |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PB10 */
  GPIO_InitStruct.Pin = GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PA11 */
  GPIO_InitStruct.Pin = GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : OLED_Reset_Pin */
  GPIO_InitStruct.Pin = OLED_Reset_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(OLED_Reset_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : OLED_CS_Pin OLED_DC_Pin */
  GPIO_InitStruct.Pin = OLED_CS_Pin|OLED_DC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  HAL_NVIC_SetPriority(EXTI2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);

  HAL_NVIC_SetPriority(EXTI3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);

  HAL_NVIC_SetPriority(EXTI4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);

  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART1) {
    if (rx_data == '\n' || rx_data == '\r') {
      // ONLY trigger if we actually have characters in the buffer
      if (rx_index > 0) {
        rx_buffer[rx_index] = '\0';
        rx_flag = 1;
      }
    }
    else if (rx_index < sizeof(rx_buffer) - 1) {
      rx_buffer[rx_index++] = rx_data;
    }
    HAL_UART_Receive_IT(&huart1, &rx_data, 1);
  }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  uint32_t current_time = HAL_GetTick();
  const uint32_t debounce_delay = 220;

  switch (GPIO_Pin) {

  case GPIO_PIN_1: // Push button 1
    if ((current_time - last_press_button1) > debounce_delay) {
      flag_button1 = 1;
      last_press_button1 = current_time;
    }
    break;

  case GPIO_PIN_2: // Push button 2
    if ((current_time - last_press_button2) > debounce_delay) {
      flag_button2 = 1;
      last_press_button2 = current_time;
    }
    break;

  case GPIO_PIN_6: // Push button 3
    if ((current_time - last_press_button3) > debounce_delay) {
      flag_button3 = 1;
      last_press_button3 = current_time;
    }
    break;

  case GPIO_PIN_5: // Push button 4
    if ((current_time - last_press_button4) > debounce_delay) {
      flag_button4 = 1;
      last_press_button4 = current_time;
    }
    break;

  case GPIO_PIN_0: // Toggle 1
    if ((current_time - last_press_toggle1) > debounce_delay) {
      flag_toggle1 = 1;
      last_press_toggle1 = current_time;
    }
    break;

  case GPIO_PIN_7: // Toggle 2
    if ((current_time - last_press_toggle2) > debounce_delay) {
      flag_toggle2 = 1;
      last_press_toggle2 = current_time;
    }
    break;

  case GPIO_PIN_3: // Paddle 1
    if ((current_time - last_press_paddle1) > debounce_delay) {
      flag_paddle1 = 1;
      last_press_paddle1 = current_time;
  }
    break;

  case GPIO_PIN_4: // Paddle 2
    if ((current_time - last_press_paddle2) > debounce_delay) {
      flag_paddle2 = 1;
      last_press_paddle2 = current_time;
    }
    break;
  }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART1) {
    // 1. Forcefully clear the Overrun (ORE) flag
    __HAL_UART_CLEAR_OREFLAG(huart);

    // 2. Force the HAL state machine back to READY
    huart->RxState = HAL_UART_STATE_READY;

    // 3. Restart the interrupt receiver
    HAL_UART_Receive_IT(&huart1, &rx_data, 1);
  }
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
