/**
 * @file can_processor.h
 */

#ifndef CAN_PROCESSOR_H
#define CAN_PROCESSOR_H 

#include <unordered_map>
#include "can_bridge.h"

extern "C"
{
    #include "can_interface.h"
}

/**
 * @brief A mutable map that contains CAN message data. For any given id `i`,
 * `observed_data[i]` is the last received data value for any message with that
 * id.
 */
extern std::unordered_map<int, double> observed_data;

/**
 * @brief Instantiates the CAN bus background application for the dashboard.
 * This is the top-level accessor for the C/C++ application for this project.
 * Once this method is invoked, it will being receiving, processing, and sending
 * CAN bus data.
 */
void start();

#endif // CAN_PROCESSOR_H