#ifndef __CONTROL_H
#define __CONTROL_H

#include "main.h"

#define X_EXTEND_CM     72.39f
#define X_COMP_CM       47.625f
#define V_EXTEND        0.0f
#define V_COMP          5.0f

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
