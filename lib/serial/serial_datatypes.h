#ifndef SERIAL_DATATYPES_H
#define SERIAL_DATATYPES_H

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define START_MARKER 0x02
#define END_MARKER 0x03

/**
 * Represents an instance of a serial message
 */
typedef struct {
    uint8_t start_marker { START_MARKER }; // constant for serial messages
    uint8_t message_id;
    uint64_t message;
    uint8_t end_marker { END_MARKER }; // constant for serial messages
} SerialMessage_t;

#endif // SERIAL_DATATYPES_H