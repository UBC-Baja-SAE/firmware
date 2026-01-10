#ifndef __CONTROL_H
#define __CONTROL_H

void SendRPMOnCan(uint32_t can_id);
void SendSpeedOnCan(uint32_t can_id);
void SendFuelOnCan(uint32_t can_id);
void SendTempOnCan(uint32_t can_id);

#endif // __CONTROL_H