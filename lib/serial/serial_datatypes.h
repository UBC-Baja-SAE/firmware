#ifndef SERIAL_DATATYPES_H
#define SERIAL_DATATYPES_H

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define MAX_MESSAGE_SIZE 1 // we only need 1 byte of data for now
#define START_MARKER 0x02
#define END_MARKER 0x03

typedef struct {
    uint8_t start_marker;
    uint8_t message_id;
    uint8_t message;
    uint8_t end_marker;
} SerialMessage;

#endif // SERIAL_DATATYPES_H