/**
 * @file can_datatypes.h
 * @brief Represents a CAN message that is sent along the CAN bus
 */

#ifndef CAN_DATATYPES_H
#define CAN_DATATYPES_H

#include <stdint.h>

typedef struct {
    uint8_t id = 0;
    uint8_t data_length = 8;
    uint64_t data = 0;
    uint8_t is_extended_id = 0;
} CANMessage;

#endif // CAN_DATATYPES_H