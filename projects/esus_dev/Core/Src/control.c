#include "control.h"
#include "main.h"
#include <string.h>
#include <stdbool.h>

extern ADC_HandleTypeDef hadc1;
extern FDCAN_HandleTypeDef hfdcan1;
extern SPI_HandleTypeDef hspi1;
extern TIM_HandleTypeDef htim3;

uint8_t pot_tx_data[2];

// ─── CAN ────────────────────────────────────────────────────────────────────

static void CAN_Transmit(uint32_t id, uint8_t *data, uint32_t len) {
    FDCAN_TxHeaderTypeDef tx_header = {
        .Identifier          = id,
        .IdType              = FDCAN_STANDARD_ID,
        .TxFrameType         = FDCAN_DATA_FRAME,
        .DataLength          = len,
        .ErrorStateIndicator = FDCAN_ESI_ACTIVE,
        .BitRateSwitch       = FDCAN_BRS_OFF,
        .FDFormat            = FDCAN_CLASSIC_CAN,
        .TxEventFifoControl  = FDCAN_NO_TX_EVENTS,
        .MessageMarker       = 0
    };
    if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &tx_header, data) != HAL_OK) {
        Error_Handler();
    }
}

// ─── POT ────────────────────────────────────────────────────────────────────

float VoltageToPosition(float voltage) {
    return X_EXTEND_CM +
           (voltage * (X_EXTEND_CM - X_COMP_CM) / (V_EXTEND - V_COMP));
}

void SendPotOnCan(uint32_t can_id) {
    const int samples = 10;
    float posSum = 0;
    ADC_ChannelConfTypeDef sConfig = {0};

    sConfig.Channel      = ADC_CHANNEL_5;
    sConfig.Rank         = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_64CYCLES_5;
    sConfig.SingleDiff   = ADC_SINGLE_ENDED;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset       = 0;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    for (int i = 0; i < samples; i++) {
        HAL_ADC_Start(&hadc1);
        if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
            uint32_t adcRaw  = HAL_ADC_GetValue(&hadc1);
            float voltage    = ((float)adcRaw * 1.8f) / 65535.0f;
            float position   = VoltageToPosition(voltage);
            posSum          += position;
        }
        HAL_ADC_Stop(&hadc1);
    }

    float posAvg    = posSum / samples;
    uint16_t posCan = (uint16_t)(posAvg * 100.0f);
    pot_tx_data[0]  = (posCan >> 8) & 0xFF;
    pot_tx_data[1]  =  posCan       & 0xFF;

    CAN_Transmit(can_id, pot_tx_data, FDCAN_DLC_BYTES_2);
}

// ─── DWT Delay ──────────────────────────────────────────────────────────────

void DWT_Init(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->LAR = 0xC5ACCE55; // Unlock DWT
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void delay_us(uint32_t us) {
    if (us == 0) return;
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * (SystemCoreClock / 1000000);
    while ((DWT->CYCCNT - start) < ticks);
}

// ─── DRV8461 ────────────────────────────────────────────────────────────────

typedef struct {
    GPIO_TypeDef *cs_port;
    uint16_t      cs_pin;
    GPIO_TypeDef *sleep_port;
    uint16_t      sleep_pin;
    GPIO_TypeDef *enable_port;
    uint16_t      enable_pin;
    GPIO_TypeDef *dir_port;
    uint16_t      dir_pin;
    GPIO_TypeDef *step_port;
    uint16_t      step_pin;
    uint8_t       trq_dac;
} DRV8461_Config;

static const DRV8461_Config motor_cfg[2] = {
    [MOTOR_NEMA17] = {
        .cs_port     = GPIOG, .cs_pin     = GPIO_PIN_12,
        .sleep_port  = GPIOD, .sleep_pin  = GPIO_PIN_4,
        .enable_port = GPIOD, .enable_pin = GPIO_PIN_5,
        .dir_port    = GPIOD, .dir_pin    = GPIO_PIN_6,
        .step_port   = GPIOG, .step_pin   = GPIO_PIN_10,
        .trq_dac     = 0x80
    },
    [MOTOR_NEMA23] = {
        .cs_port     = GPIOB, .cs_pin     = GPIO_PIN_3,
        .sleep_port  = GPIOE, .sleep_pin  = GPIO_PIN_5,
        .enable_port = GPIOE, .enable_pin = GPIO_PIN_4,
        .dir_port    = GPIOE, .dir_pin    = GPIO_PIN_3,
        .step_port   = GPIOE, .step_pin   = GPIO_PIN_2,
        .trq_dac     = 0x80
    }
};

static void CS_Low(DRV8461_Motor m) {
    HAL_GPIO_WritePin(motor_cfg[m].cs_port, motor_cfg[m].cs_pin, GPIO_PIN_RESET);
}

static void CS_High(DRV8461_Motor m) {
    HAL_GPIO_WritePin(motor_cfg[m].cs_port, motor_cfg[m].cs_pin, GPIO_PIN_SET);
}

void DRV8461_Write(DRV8461_Motor motor, uint8_t reg, uint8_t data) {
    uint8_t tx[2] = { reg & 0x3F, data };
    CS_Low(motor);
    delay_us(1);
    HAL_SPI_Transmit(&hspi1, tx, 2, 10);
    delay_us(1);
    CS_High(motor);
}

uint8_t DRV8461_Read(DRV8461_Motor motor, uint8_t reg) {
    uint8_t tx[2] = { DRV8461_READ_FLAG | (reg & 0x3F), 0x00 };
    uint8_t rx[2] = {0};
    CS_Low(motor);
    delay_us(1);
    HAL_SPI_TransmitReceive(&hspi1, tx, rx, 2, 10);
    delay_us(1);
    CS_High(motor);
    return rx[1];
}

static void DRV8461_InitMotor(DRV8461_Motor m) {
    const DRV8461_Config *c = &motor_cfg[m];

    HAL_GPIO_WritePin(c->enable_port, c->enable_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(c->sleep_port,  c->sleep_pin,  GPIO_PIN_RESET);
    HAL_Delay(2);

    HAL_GPIO_WritePin(c->sleep_port, c->sleep_pin, GPIO_PIN_SET);
    HAL_Delay(10);

    uint8_t ctrl2 = DRV8461_Read(m, DRV8461_REG_CTRL2);
    DRV8461_Write(m, DRV8461_REG_CTRL2, ctrl2 | 0x80); // Clear faults
    HAL_Delay(1);

    DRV8461_Write(m, DRV8461_REG_CTRL1, 0x05); // 1/16 microstepping
    DRV8461_Write(m, DRV8461_REG_CTRL3, c->trq_dac);
    DRV8461_Write(m, DRV8461_REG_CTRL4, 0x00); // Use hardware ENABLE pin
    DRV8461_Write(m, DRV8461_REG_CTRL6, 0x01); // Gain = 5

    HAL_GPIO_WritePin(c->enable_port, c->enable_pin, GPIO_PIN_SET);
    HAL_Delay(1);
}

void DRV8461_InitAll(void) {
    CS_High(MOTOR_NEMA17);
    CS_High(MOTOR_NEMA23);
    HAL_Delay(10);

    DRV8461_InitMotor(MOTOR_NEMA17);
    HAL_Delay(10);
    DRV8461_InitMotor(MOTOR_NEMA23);
}

uint8_t DRV8461_GetFault(DRV8461_Motor motor) {
    return DRV8461_Read(motor, DRV8461_REG_FAULT);
}

void DRV8461_ClearFault(DRV8461_Motor motor) {
    uint8_t ctrl2 = DRV8461_Read(motor, DRV8461_REG_CTRL2);
    DRV8461_Write(motor, DRV8461_REG_CTRL2, ctrl2 | 0x80);
}

void DRV8461_Enable(DRV8461_Motor motor, uint8_t enable) {
    HAL_GPIO_WritePin(motor_cfg[motor].enable_port,
                      motor_cfg[motor].enable_pin,
                      enable ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void DRV8461_SetDirection(DRV8461_Motor motor, uint8_t dir) {
    HAL_GPIO_WritePin(motor_cfg[motor].dir_port,
                      motor_cfg[motor].dir_pin,
                      dir ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void DRV8461_Step(DRV8461_Motor motor) {
    HAL_GPIO_WritePin(motor_cfg[motor].step_port, motor_cfg[motor].step_pin, GPIO_PIN_SET);
    delay_us(2);
    HAL_GPIO_WritePin(motor_cfg[motor].step_port, motor_cfg[motor].step_pin, GPIO_PIN_RESET);
}

// ─── Timer-based Stepping ───────────────────────────────────────────────────

volatile uint32_t nema17_steps_remaining = 0;
volatile uint32_t nema23_steps_remaining = 0;

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM3) {
        if (nema17_steps_remaining > 0) {
            DRV8461_Step(MOTOR_NEMA17);
            nema17_steps_remaining--;
        }
        if (nema23_steps_remaining > 0) {
            DRV8461_Step(MOTOR_NEMA23);
            nema23_steps_remaining--;
        }
    }
}

void RunMotorTimerTest(void) {
    DRV8461_SetDirection(MOTOR_NEMA17, 1);
    DRV8461_SetDirection(MOTOR_NEMA23, 0);

    nema17_steps_remaining = 3200; // 1 rotation at 1/16 microstepping
    nema23_steps_remaining = 3200;

    HAL_TIM_Base_Start_IT(&htim3);

    // Wait for completion
    while (nema17_steps_remaining > 0 || nema23_steps_remaining > 0) {
        __NOP();
    }

    HAL_TIM_Base_Stop_IT(&htim3);
}
