#ifndef __CONTROL_H
#define __CONTROL_H

#include "main.h"

#define X_EXTEND_CM     59.5f
#define X_COMP_CM       34.5f
#define V_EXTEND        0.0f
#define V_COMP          1.8f

// ICM-42670-P Definitions
#define ICM_ADDR         (0x68 << 1)
#define ACCEL_DATA_START 0x1F
#define GYRO_DATA_START  0x19
#define PWR_MGMT0        0x11

void IMU_Init(void);
float VoltageToPosition(float voltage);
void SendPotOnCan(uint32_t can_id);
void SendAccelOnCan(uint32_t can_id);
void SendGyroOnCan(uint32_t can_id);
void SendStrainOnCan(uint32_t can_id, uint32_t channel);

#endif // __CONTROL_H
