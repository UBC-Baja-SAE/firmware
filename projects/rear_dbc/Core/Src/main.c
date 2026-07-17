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
#include "cmsis_os.h"
#include "fdcan.h"
#include "i2c.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
HAL_GPIO_WritePin(GPIOE, GPIO_PIN_6, GPIO_PIN_RESET);
  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

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
  MX_FDCAN1_Init();
  MX_TIM3_Init();
  MX_TIM1_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */


  HAL_TIM_Base_Start_IT(&htim1);
  HAL_TIM_IC_Start_IT(&htim1, TIM_CHANNEL_2);

  HAL_TIM_Base_Start_IT(&htim3);
  HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_2);

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();  /* Call init function for freertos objects (in cmsis_os2.c) */
  MX_FREERTOS_Init();

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = 64;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 12;
  RCC_OscInitStruct.PLL.PLLP = 1;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

#define TIM1_CLOCK_FREQ         240000000UL   // APB1 Timer Clock @240MHz
#define TIM3_CLOCK_FREQ         240000000UL   // APB1 Timer Clock @240MHz
#define TIM1_PERIOD_TICKS       65536ULL      // 2^16 for 16-bit TIM1
#define TIM3_PERIOD_TICKS       65536ULL      // 2^16 for 16-bit TIM3

#define MAGNET_DEBOUNCE_TIME_MS 5U
#define SPARK_DEBOUNCE_TIME_MS  10U
#define PULSES_PER_REV          2.0f

#define SPEED_SMOOTHING         0.2f
#define TIRE_CIRCUMFERENCE_KM   0.001963f
#define MAX_SPEED_KMH           150U
#define MAX_RPM_LIMIT           20000U

#define SENSOR_TIMEOUT_MS 5000U

static volatile uint32_t previous_capture     = 0;
static volatile uint8_t  speed_first_pulse    = 1;
volatile uint64_t tim1_overflow_count  = 0;
static volatile float    smoothed_speed_freq  = 0.0f;

volatile uint32_t last_magnet_time     = 0;
volatile uint32_t last_spark_time      = 0;

static volatile uint32_t tach_previous_capture = 0;
static volatile uint8_t  tach_first_pulse      = 1;
volatile uint64_t tim3_overflow_count  = 0;
static volatile float    tach_smoothed_freq    = 0.0f;

// Outputs
volatile uint32_t speedometer_kmh = 0;
volatile uint32_t tachometer_rpm = 0;

void Speedometer_OverflowISR(void)
{
  tim1_overflow_count++;
}

void Tachometer_OverflowISR(void)
{
  tim3_overflow_count++;
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
  /* ---------------- SPEEDOMETER (TIM1) ---------------- */
  if (htim->Instance == TIM1 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2)
  {
    uint32_t now = HAL_GetTick();

    if ((now - last_magnet_time) < MAGNET_DEBOUNCE_TIME_MS)
      return;
    last_magnet_time = now;

    uint32_t cap = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);

    if (speed_first_pulse)
    {
      previous_capture    = cap;
      tim1_overflow_count = 0;
      speed_first_pulse   = 0;
      return;
    }

    uint64_t diff = (tim1_overflow_count * TIM1_PERIOD_TICKS)
                  + (uint64_t)cap
                  - (uint64_t)previous_capture;

    previous_capture    = cap;
    tim1_overflow_count = 0;

    if (diff > 0ULL)
    {
      float instant_freq = (float)TIM1_CLOCK_FREQ / (float)diff;

      smoothed_speed_freq = (instant_freq * SPEED_SMOOTHING)
                          + (smoothed_speed_freq * (1.0f - SPEED_SMOOTHING));

      uint32_t kmh = (uint32_t)(smoothed_speed_freq
                               * TIRE_CIRCUMFERENCE_KM
                               * 3600.0f);

      speedometer_kmh = (kmh > MAX_SPEED_KMH) ? MAX_SPEED_KMH : kmh;
    }
  }

  /* ---------------- TACHOMETER (TIM3) ---------------- */
  /* ---------------- TACHOMETER (TIM3) ---------------- */
  else if (htim->Instance == TIM3 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2)
  {
    uint32_t now = HAL_GetTick();

    // SOFTWARE DEBOUNCE: Ignores the BJT high-frequency ringing
    if ((now - last_spark_time) < SPARK_DEBOUNCE_TIME_MS) {
      return;
    }
    last_spark_time = now;

    uint32_t cap = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);

    if (tach_first_pulse)
    {
      tach_previous_capture = cap;
      tim3_overflow_count = 0;
      tach_first_pulse = 0;
      return;
    }

    // New safe multi-overflow math
    uint64_t diff = (tim3_overflow_count * TIM3_PERIOD_TICKS)
                  + (uint64_t)cap
                  - (uint64_t)tach_previous_capture;

    tach_previous_capture = cap;
    tim3_overflow_count = 0;

    if (diff > 0ULL)
    {
      float instant_freq = (float)TIM3_CLOCK_FREQ / (float)diff;

      tach_smoothed_freq = (instant_freq * SPEED_SMOOTHING)
                          + (tach_smoothed_freq * (1.0f - SPEED_SMOOTHING));

      // Calculate true RPM using your old logic
      uint32_t calculated_rpm = (uint32_t)((tach_smoothed_freq * 60.0f) / PULSES_PER_REV);

      // Cap at max limit
      tachometer_rpm = (calculated_rpm > MAX_RPM_LIMIT) ? MAX_RPM_LIMIT : calculated_rpm;
    }
  }
}

void Check_Sensor_Timeouts(void)
{
  uint32_t now = HAL_GetTick();

  /* Check Speedometer Timeout */
  if ((now - last_magnet_time) > SENSOR_TIMEOUT_MS)
  {
    speedometer_kmh = 0;
    smoothed_speed_freq = 0.0f;
    speed_first_pulse = 1; // Crucial: Resets state so the next movement doesn't calculate a massive time gap
  }

  /* Check Tachometer Timeout */
  if ((now - last_spark_time) > SENSOR_TIMEOUT_MS)
  {
    tachometer_rpm = 0;
    tach_smoothed_freq = 0.0f;
    tach_first_pulse = 1;
  }
}
/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */
  if (htim->Instance == TIM1)
  {
    Speedometer_OverflowISR();
  }
  if (htim->Instance == TIM3)
  {
    Tachometer_OverflowISR();
  }

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
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
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
