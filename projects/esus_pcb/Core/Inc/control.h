#ifndef __CONTROL_H
#define __CONTROL_H

#include "main.h"

#define X_EXTEND_CM 59.5f
#define X_COMP_CM 34.5f
#define V_EXTEND 0.0f
#define V_COMP 1.8f

// ICM-42670-P Definitions
#define ICM_ADDR (0x68 << 1)
#define ACCEL_DATA_START 0x0B
#define GYRO_DATA_START 0x11
// Register Map
#define PWR_MGMT0 0x1F
#define GYRO_CONFIG0 0x20
#define ACCEL_CONFIG0 0x20
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
#define DRV_REG_DIAG2     0x02
#define DRV_REG_CTRL1     0x04
#define DRV_REG_CTRL2     0x05

// Motor Identifiers
typedef enum {
    MOTOR_NEMA17 = 0,
    MOTOR_NEMA23 = 1
} MotorID_t;

typedef struct {
    int32_t nema17_target;
    int32_t nema23_target;
} StepperSetting_t;

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

void Motors_Init(void);
void Motor_Step(MotorID_t motor, uint8_t direction, uint32_t steps);
void Motors_Step_Simultaneous(int32_t move17, int32_t move23);
void Motor_GoTo_Setting(uint8_t setting_id);
void Motor_Calibrate_All(void);
void Motor_Calibrate(MotorID_t motor);
uint16_t DRV8461_Transfer(MotorID_t motor, uint8_t addr, uint8_t data);
uint8_t Motor_CheckAndRecover(void);

#endif // __CONTROL_H
