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
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "usart.h"
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
void ICM42670_Init(void);
void ICM42670_ReadData(void);

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
  MX_DMA_Init();
  MX_UART4_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);

  // HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1);

  HAL_ADCEx_Calibration_Start(&hadc1, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED);
  HAL_ADC_Start_IT(&hadc1);

  ICM42670_Init();
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Macro to configure the PLL clock source
  */
  __HAL_RCC_PLL_PLLSOURCE_CONFIG(RCC_PLLSOURCE_HSI);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = 64;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* ── speedo constants ────────────────────────────────────────────── */
#define TIM2_CLOCK_FREQ         64000000UL   // HSI @ 64 MHz, prescaler = 0
#define TIMER_PERIOD_TICKS      4294967296ULL // 2^32 for 32-bit TIM2
#define MAGNET_DEBOUNCE_TIME_MS 5U
#define SPEED_SMOOTHING         0.2f
#define TIRE_CIRCUMFERENCE_KM   0.001963f
#define MAX_SPEED_KMH           150U

/* ── imu constants ────────────────────────────────────────────── */
#define ICM42670_ADDR           (0x68 << 1)
#define ICM42670_REG_WHO_AM_I   0x75
#define ICM42670_REG_PWR_MGMT0  0x1F
#define ICM42670_REG_GYRO_CONFIG0  0x20
#define ICM42670_REG_ACCEL_CONFIG0 0x21

/* Raw output registers */
#define ICM42670_REG_TEMP_DATA1    0x09
#define ICM42670_REG_ACCEL_DATA_X1 0x0B
#define ICM42670_REG_GYRO_DATA_X1  0x11

/* ── state (ISR-owned) ─────────────────────────────────────────────── */
static volatile uint32_t previous_capture     = 0;
static volatile uint8_t  speed_first_pulse    = 1;
static volatile uint64_t tim2_overflow_count  = 0;  // exposed for PeriodElapsed
static volatile float    smoothed_speed_freq  = 0.0f;

volatile uint32_t last_magnet_time     = 0;

volatile int16_t imu_accel_x = 0, imu_accel_y = 0, imu_accel_z = 0;
volatile int16_t imu_gyro_x  = 0, imu_gyro_y  = 0, imu_gyro_z  = 0;


/* ── speedometer output ────────────────────────────────── */
volatile uint32_t speedometer_kmh = 0;

/* ── lin pot output ────────────────────────────────── */
volatile uint32_t linpot_raw_value = 0;


static uint8_t ICM42670_WriteReg(uint8_t reg, uint8_t val)
{
  uint8_t buf[2] = { reg, val };
  return HAL_I2C_Master_Transmit(&hi2c1, ICM42670_ADDR, buf, 2, 10);
}

static uint8_t ICM42670_ReadReg(uint8_t reg)
{
  uint8_t val = 0;
  HAL_I2C_Master_Transmit(&hi2c1, ICM42670_ADDR, &reg, 1, 10);
  HAL_I2C_Master_Receive(&hi2c1, ICM42670_ADDR, &val, 1, 10);
  return val;
}




/* Called from HAL_TIM_PeriodElapsedCallback in main.c */
void Speedometer_OverflowISR(void)
{
  tim2_overflow_count++;
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM2 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
  {
    uint32_t now = HAL_GetTick();

    if ((now - last_magnet_time) < MAGNET_DEBOUNCE_TIME_MS)
      return;
    last_magnet_time = now;

    uint32_t cap = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);

    if (speed_first_pulse)
    {
      previous_capture    = cap;
      tim2_overflow_count = 0;
      speed_first_pulse   = 0;
      return;
    }

    uint64_t diff = (tim2_overflow_count * TIMER_PERIOD_TICKS)
                  + (uint64_t)cap
                  - (uint64_t)previous_capture;

    previous_capture    = cap;
    tim2_overflow_count = 0;

    if (diff > 0ULL)
    {
      float instant_freq = (float)TIM2_CLOCK_FREQ / (float)diff;

      smoothed_speed_freq = (instant_freq * SPEED_SMOOTHING)
                          + (smoothed_speed_freq * (1.0f - SPEED_SMOOTHING));

      uint32_t kmh = (uint32_t)(smoothed_speed_freq
                               * TIRE_CIRCUMFERENCE_KM
                               * 3600.0f);

      speedometer_kmh = (kmh > MAX_SPEED_KMH) ? MAX_SPEED_KMH : kmh;
    }
  }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
  if (hadc->Instance == ADC1)
  {
    linpot_raw_value = HAL_ADC_GetValue(hadc);
    HAL_ADC_Start_IT(hadc);  // re-arm for next conversion
  }
}

void ICM42670_Init(void)
{
  uint8_t who = ICM42670_ReadReg(ICM42670_REG_WHO_AM_I);
  if (who != 0x67) { return; }

  ICM42670_WriteReg(ICM42670_REG_PWR_MGMT0, 0x0F);
  HAL_Delay(10);

  ICM42670_WriteReg(ICM42670_REG_GYRO_CONFIG0, 0x04);
  ICM42670_WriteReg(ICM42670_REG_ACCEL_CONFIG0, 0x24);
}

void ICM42670_ReadData(void)
{
  uint8_t buf[12] = {0};
  uint8_t reg = ICM42670_REG_ACCEL_DATA_X1;

  HAL_StatusTypeDef status;

  status = HAL_I2C_Master_Transmit(&hi2c1, ICM42670_ADDR, &reg, 1, 10);
  if (status != HAL_OK)
  {
    HAL_I2C_DeInit(&hi2c1);
    HAL_Delay(5);
    HAL_I2C_Init(&hi2c1);
    return;  // skip this cycle, don't update imu_accel/gyro globals
  }

  status = HAL_I2C_Master_Receive(&hi2c1, ICM42670_ADDR, buf, 12, 10);
  if (status != HAL_OK)
  {
    HAL_I2C_DeInit(&hi2c1);
    HAL_Delay(5);
    HAL_I2C_Init(&hi2c1);
    return;
  }

  // Only update globals if read succeeded
  imu_accel_x = (int16_t)(buf[0]  << 8 | buf[1]);
  imu_accel_y = (int16_t)(buf[2]  << 8 | buf[3]);
  imu_accel_z = (int16_t)(buf[4]  << 8 | buf[5]);
  imu_gyro_x  = (int16_t)(buf[6]  << 8 | buf[7]);
  imu_gyro_y  = (int16_t)(buf[8]  << 8 | buf[9]);
  imu_gyro_z  = (int16_t)(buf[10] << 8 | buf[11]);
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
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */
  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */
  if (htim->Instance == TIM2)
  {
    Speedometer_OverflowISR();
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
