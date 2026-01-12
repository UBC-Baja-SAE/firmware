#ifndef __CONTROL_H
#define __CONTROL_H

#include "main.h"

extern volatile uint32_t measured_frequency;

void SendFrequencyOnCan(uint32_t can_id); //new!

#endif // __CONTROL_H
