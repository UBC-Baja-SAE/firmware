/**
 * @file can_message.h
 * @brief This file provides the structure for a abstract CAN message.
 */

#ifndef CAN_MESSAGE_H
#define CAN_MESSAGE_H

#include <stdint.h>

/**
 * @brief This is the abstract CAN message for the dashboard, which is agnostic
 * to the underlying CAN data frame used for communication through the Linux
 * kernel.
 */
typedef struct
{
    int id;
    int size;
    uint8_t data[8];
} CAN_Message;

#endif // CAN_MESSAGE_H