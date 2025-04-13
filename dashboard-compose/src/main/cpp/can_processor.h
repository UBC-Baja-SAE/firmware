/**
 * @file can_processor.h
 */

#ifndef CAN_PROCESSOR_H
#define CAN_PROCESSOR_H 

extern "C"
{
    #include "can_interface.h"
    #include "can_bridge.h"

    void start();
}

void poll();


#endif // CAN_PROCESS_H