#include <stdint.h>

#define MAX_DATA_LENGTH 8

typedef struct {
    uint12_t id;
    uint8_t data_length;
    uint8_t data[MAX_DATA_LENGTH];
    uint8_t is_extended_id = 0;
} CANMessage;

