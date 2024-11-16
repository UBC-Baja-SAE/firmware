/**
 * @file can_transceiver.h
 * @brief Represents the data-link layer, and provides an API to physically
 * transmit and receive CAN bus messages 
 */

#ifndef CAN_TRANSCEIVER_H
#define CAN_TRANSCEIVER_H

#include "can_datatypes.h"

/**
 * @brief Transmits a message onto the CAN bus
 * 
 * @param msg the CAN bus data frame to be sent
 */
void transmitCANMessage(CANMessage msg);

/**
 * @brief Receives a message from the CAN bus
 * 
 * @return the data currently being transmitted onto the CAN bus
 */
CANMessage receiveCANMessage();

#endif //CAN_TRANSCEIVER_H