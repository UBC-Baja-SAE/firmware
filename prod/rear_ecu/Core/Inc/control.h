#ifndef __CONTROL_H
#define __CONTROL_H

#include "main.h"

extern volatile float measured_speedometer_frequency;
extern volatile float measured_tachometer_frequency;

void SendSpeedOnCan(uint32_t can_id); //new!

void SendTachometerOnCan(uint32_t can_id);

#endif // __CONTROL_H
