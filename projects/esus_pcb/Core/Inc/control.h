#ifndef __CONTROL_H
#define __CONTROL_H

#include "main.h"

#define X_EXTEND_CM     72.39f
#define X_COMP_CM       47.625f
#define V_EXTEND        0.0f
#define V_COMP          5.0f

float VoltageToPosition(float voltage);
void SendPotOnCan(uint32_t can_id);
void SendAccelOnCan(uint32_t can_id);
void SendGyroOnCan(uint32_t can_id);
void SendStrainOnCan(uint32_t can_id);

#endif // __CONTROL_H
