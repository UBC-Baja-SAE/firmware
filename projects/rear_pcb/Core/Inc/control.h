#ifndef __CONTROL_H
#define __CONTROL_H

#include <stdint.h>
#include "main.h"
//#include "stm32h7xx_hal.h"

void SendRPMOnCan(uint32_t can_id);
void SendSpeedOnCan(uint32_t can_id);
void SendFuelOnCan(uint32_t can_id);
void SendTempOnCan(uint32_t can_id);

#endif // __CONTROL_H
