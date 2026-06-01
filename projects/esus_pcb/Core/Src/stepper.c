#include "stepper.h"
#include "main.h"

extern SPI_HandleTypeDef hspi1;

// Motor definitions matching your schematic
static StepperMotor_t motors[2] = {
    // NEMA17
    {
        .nsleep_port = GPIOD, .nsleep_pin = GPIO_PIN_4,
        .enable_port = GPIOD, .enable_pin = GPIO_PIN_5,
        .dir_port    = GPIOD, .dir_pin    = GPIO_PIN_6,
        .step_port   = GPIOG, .step_pin   = GPIO_PIN_10,
        .nscs_port   = GPIOG, .nscs_pin   = GPIO_PIN_12,
        .nfault_port = GPIOG, .nfault_pin = GPIO_PIN_13,
    },
    // NEMA23
    {
        .nsleep_port = GPIOE, .nsleep_pin = GPIO_PIN_5,
        .enable_port = GPIOE, .enable_pin = GPIO_PIN_4,
        .dir_port    = GPIOE, .dir_pin    = GPIO_PIN_3,
        .step_port   = GPIOE, .step_pin   = GPIO_PIN_2,
        .nscs_port   = GPIOB, .nscs_pin   = GPIO_PIN_3,
        .nfault_port = GPIOG, .nfault_pin = GPIO_PIN_14,
    }
};

// ── helpers ───────────────────────────────────────────────────────────────────

static void delay_us(uint32_t us)
{
    uint32_t ticks = (HAL_RCC_GetHCLKFreq() / 1000000U) * us;
    for (volatile uint32_t i = 0; i < ticks / 4; i++);
}

// ── SPI core ──────────────────────────────────────────────────────────────────
// DRV8461 SPI frame (16-bit, MSB first):
//   bit15:    R/W  — 1=read, 0=write
//   bits14:8  ADDR — 7-bit register address
//   bits7:0   DATA — register data
//
// HAL_SPI_TransmitReceive on STM32H7 with 16-bit data size:
//   - pData pointers are treated as uint16_t arrays
//   - Size parameter is number of 16-bit FRAMES (not bytes)

static uint16_t SPI_Transfer(MotorID_t id, uint16_t tx_word)
{
    uint16_t rx_word = 0;

    // CS assert — SCLK must already be low (guaranteed by CPOL=0 idle)
    HAL_GPIO_WritePin(motors[id].nscs_port, motors[id].nscs_pin, GPIO_PIN_RESET);

    // Size=1 means 1 frame of 16 bits on the H7 SPI peripheral
    HAL_SPI_TransmitReceive(&hspi1, (uint8_t*)&tx_word, (uint8_t*)&rx_word, 1, 10);

    // CS deassert — DRV8461 requires >=500ns between frames
    HAL_GPIO_WritePin(motors[id].nscs_port, motors[id].nscs_pin, GPIO_PIN_SET);
    delay_us(1); // 1us >> 500ns minimum

    return rx_word;
}

// ── DRV8461 register access ───────────────────────────────────────────────────

uint16_t DRV8461_ReadReg(MotorID_t id, uint8_t reg)
{
    // bit15=1 (read), bits[14:8]=addr, bits[7:0]=0x00 dummy
    uint16_t tx = (uint16_t)(0x8000 | ((reg & 0x7F) << 8) | 0x00);
    uint16_t rx = SPI_Transfer(id, tx);
    // Response: bits[15:8] = status byte, bits[7:0] = register data
    return rx & 0x00FF;
}

void DRV8461_WriteReg(MotorID_t id, uint8_t reg, uint8_t data)
{
    // bit15=0 (write), bits[14:8]=addr, bits[7:0]=data
    uint16_t tx = (uint16_t)(0x0000 | ((reg & 0x7F) << 8) | data);
    SPI_Transfer(id, tx);
}

// ── public API ────────────────────────────────────────────────────────────────

void Stepper_Init(void)
{
    for (int i = 0; i < 2; i++) {
        StepperMotor_t *m = &motors[i];

        HAL_GPIO_WritePin(m->nscs_port,   m->nscs_pin,   GPIO_PIN_SET);   // CS idle high
        HAL_GPIO_WritePin(m->nsleep_port, m->nsleep_pin, GPIO_PIN_RESET); // hold in sleep during init
        HAL_GPIO_WritePin(m->enable_port, m->enable_pin, GPIO_PIN_RESET); // outputs off
        HAL_GPIO_WritePin(m->dir_port,    m->dir_pin,    GPIO_PIN_SET);   // default forward
        HAL_GPIO_WritePin(m->step_port,   m->step_pin,   GPIO_PIN_RESET); // STEP idle low
    }

    // Wake both drivers — nSLEEP high, then wait for charge pump to stabilize
    for (int i = 0; i < 2; i++) {
        HAL_GPIO_WritePin(motors[i].nsleep_port, motors[i].nsleep_pin, GPIO_PIN_SET);
    }
    HAL_Delay(10); // datasheet: 1ms typical, use 10ms to be safe

    // Enable internal VREF and set full scale current
    // CTRL3 register (0x05): set VREF_INT_EN bit (bit 0 = 1)
    DRV8461_WriteReg(MOTOR_NEMA23, DRV8461_REG_CTRL3, 0x01);
    HAL_Delay(1);

    // CTRL1 register (0x03): set TRQ_DAC to full scale (bits [7:3] = 11111)
    DRV8461_WriteReg(MOTOR_NEMA23, DRV8461_REG_CTRL1, 0xF8);
    HAL_Delay(1);

    // Clear any power-on faults on both drivers
    DRV8461_WriteReg(MOTOR_NEMA17, DRV8461_REG_CTRL1, 0x80); // CLR_FLT
    DRV8461_WriteReg(MOTOR_NEMA23, DRV8461_REG_CTRL1, 0x80);
    HAL_Delay(1);
}

void Stepper_Enable(MotorID_t id)
{
    HAL_GPIO_WritePin(motors[id].enable_port,
                      motors[id].enable_pin, GPIO_PIN_SET); // active high
}

void Stepper_Disable(MotorID_t id)
{
    HAL_GPIO_WritePin(motors[id].enable_port,
                      motors[id].enable_pin, GPIO_PIN_RESET);
}

void Stepper_Sleep(MotorID_t id)
{
    HAL_GPIO_WritePin(motors[id].nsleep_port,
                      motors[id].nsleep_pin, GPIO_PIN_RESET); // active low
}

void Stepper_Wake(MotorID_t id)
{
    HAL_GPIO_WritePin(motors[id].nsleep_port,
                      motors[id].nsleep_pin, GPIO_PIN_SET);
    HAL_Delay(10);
}

void Stepper_SetDir(MotorID_t id, uint8_t forward)
{
    HAL_GPIO_WritePin(motors[id].dir_port, motors[id].dir_pin,
                      forward ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void Stepper_Step(MotorID_t id, uint32_t steps, uint32_t delay_us_period)
{
    // Minimum STEP pulse width per DRV8461 datasheet is 1us high, 1us low
    uint32_t half = delay_us_period / 2;
    if (half < 2) half = 2; // enforce minimum

    for (uint32_t i = 0; i < steps; i++) {
        HAL_GPIO_WritePin(motors[id].step_port, motors[id].step_pin, GPIO_PIN_SET);
        delay_us(half);
        HAL_GPIO_WritePin(motors[id].step_port, motors[id].step_pin, GPIO_PIN_RESET);
        delay_us(half);
    }
}

uint8_t Stepper_IsFault(MotorID_t id)
{
    // nFAULT is open-drain active-low: LOW = fault present
    return (HAL_GPIO_ReadPin(motors[id].nfault_port,
                             motors[id].nfault_pin) == GPIO_PIN_RESET);
}

void Stepper_ClearFault(MotorID_t id)
{
    // Write CLR_FLT (bit7) to CTRL1 — self-clearing on the device
    uint8_t ctrl1 = (uint8_t)(DRV8461_ReadReg(id, DRV8461_REG_CTRL1) & 0xFF);
    DRV8461_WriteReg(id, DRV8461_REG_CTRL1, ctrl1 | 0x80);
    HAL_Delay(1);
}