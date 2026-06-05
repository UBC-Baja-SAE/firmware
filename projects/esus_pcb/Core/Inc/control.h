#ifndef __CONTROL_H
#define __CONTROL_H

#include "main.h"

#define X_EXTEND_CM 62.0f
#define X_COMP_CM   37.0f
#define V_COMP      0.0f    // 0V = fully compressed
#define V_EXTEND    1.8f    // 1.8V = fully extended

// ICM-42670-P Definitions
#define ICM_ADDR (0x68 << 1)
#define ACCEL_DATA_START 0x0B
#define GYRO_DATA_START 0x11
// Register Map
#define PWR_MGMT0 0x1F
#define GYRO_CONFIG0 0x20
#define ACCEL_CONFIG0 0x21
#define WHO_AM_I 0x75
#define WHO_AM_I_VAL 0x67

// I2C Recovery Configuration
#define I2C_RECOVERY_MAX_CLOCKS 9
#define IMU_INIT_RETRY_COUNT 3
#define IMU_STARTUP_DELAY_MS 100

// DRV8461 Stepper Definitions
/* --- NEMA 17 Pins (Corner A) --- */
#define N17_SLEEP_PIN   GPIO_PIN_4
#define N17_SLEEP_PORT  GPIOD
#define N17_ENABLE_PIN  GPIO_PIN_5
#define N17_ENABLE_PORT GPIOD
#define N17_DIR_PIN     GPIO_PIN_6
#define N17_DIR_PORT    GPIOD
#define N17_STEP_PIN    GPIO_PIN_10
#define N17_STEP_PORT   GPIOG
#define N17_NCS_PIN     GPIO_PIN_12
#define N17_NCS_PORT    GPIOG
#define N17_FAULT_PIN   GPIO_PIN_13
#define N17_FAULT_PORT  GPIOG

/* --- NEMA 23 Pins (Corner B) --- */
#define N23_SLEEP_PIN   GPIO_PIN_5
#define N23_SLEEP_PORT  GPIOE
#define N23_ENABLE_PIN  GPIO_PIN_4
#define N23_ENABLE_PORT GPIOE
#define N23_DIR_PIN     GPIO_PIN_3
#define N23_DIR_PORT    GPIOE
#define N23_STEP_PIN    GPIO_PIN_2
#define N23_STEP_PORT   GPIOE
#define N23_NCS_PIN     GPIO_PIN_3
#define N23_NCS_PORT    GPIOB
#define N23_FAULT_PIN   GPIO_PIN_14
#define N23_FAULT_PORT  GPIOG

// SPI Registers (DRV8461)
#define DRV_REG_FAULT     0x00
#define DRV_REG_DIAG1     0x01
#define DRV_REG_DIAG2     0x02
#define DRV_REG_DIAG3     0x03
#define DRV_REG_CTRL1     0x04
#define DRV_REG_CTRL2     0x05
#define DRV_REG_CTRL3     0x06
#define DRV_REG_CTRL4     0x07
#define DRV_REG_CTRL5     0x08  // ← Slew rate control
#define DRV_REG_CTRL6     0x09  // ← Spread spectrum / dither
#define DRV_REG_CTRL7     0x0A  // ← StallGuard config
#define DRV_REG_CTRL8     0x0B  // ← ATD control
#define DRV_REG_CTRL9     0x0C
#define DRV_REG_CTRL10    0x0D
#define DRV_REG_CTRL11    0x0E
#define DRV_REG_CTRL12    0x0F
#define DRV_REG_CTRL13    0x10

// Function Declarations
HAL_StatusTypeDef I2C_BusRecovery(void);
HAL_StatusTypeDef IMU_Init(void);
uint8_t IMU_IsResponding(void);
float VoltageToPosition(float voltage);
void SendPotOnCan(uint32_t can_id);
void SendGyroOnCan(uint32_t can_id);
void SendAccelOnCan(uint32_t can_id);
void SendStrainOnCan(uint32_t can_id, uint32_t channel);
void SendEsusStatusOnCan(uint32_t can_id);

#endif // __CONTROL_H
