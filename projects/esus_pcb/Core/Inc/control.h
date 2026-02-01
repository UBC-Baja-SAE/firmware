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

// Function Declarations
HAL_StatusTypeDef I2C_BusRecovery(void);
HAL_StatusTypeDef IMU_Init(void);
uint8_t IMU_IsResponding(void);
float VoltageToPosition(float voltage);
void SendPotOnCan(uint32_t can_id);
void SendGyroOnCan(uint32_t can_id);
void SendAccelOnCan(uint32_t can_id);
void SendStrainOnCan(uint32_t can_id, uint32_t channel);

#endif // __CONTROL_H
