//
// Created by Aleks on 9/20/2026.
//

#ifndef FIRMWARE_HELPERS_H
#define FIRMWARE_HELPERS_H

#include <stdint.h>
#include <stdbool.h>

// IR Sensor Addresses
#define IR_SLAVE_ADDR 0x5A
#define AMB_POLL_ADDR 0x06
#define OBJ_POLL_ADDR 0x07

// 1. The packed struct replacing the 5-byte array
typedef struct __attribute__((packed)) {
    uint8_t slave_write_addr;
    uint8_t register_addr;
    uint8_t slave_read_addr;
    uint8_t data_lsb;
    uint8_t data_msb;
} crc_payload_t;

// 2. Function prototype for the validator
// We pass the target register and the 3-byte DMA buffer to keep the caller clean
bool MLX90614_VerifyData(uint8_t target_reg, volatile uint8_t *dma_rx_buffer);

#endif //FIRMWARE_HELPERS_H

