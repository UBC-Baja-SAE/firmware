#ifndef __CONTROL_H
#define __CONTROL_H

#include "main.h"

#define X_EXTEND_CM 59.5f
#define X_COMP_CM 34.5f
#define V_EXTEND 0.0f
#define V_COMP 1.8f

void SendPotOnCan(uint32_t can_id);
void SendGyroOnCan(uint32_t can_id);
void SendAccelOnCan(uint32_t can_id);


#endif // __CONTROL_H
