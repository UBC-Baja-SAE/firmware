//
// Created by Aleks on 9/20/2026.
//

#include "helpers.h"
#include <stdint.h>

// Calculates the SMBus (sub i2c) crc byte passed on each IR sensor read
// IR sensor task verifies the calculated crc is matching the sensed one
static uint8_t calculate_smbus_pec(uint8_t *data, uint8_t len) {
  uint8_t crc = 0;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; bit++) {
      if (crc & 0x80) {
        // 0x07 is the polynomial x^8 + x^2 + x^1 + 1
        crc = (crc << 1) ^ 0x07;
      } else {
        crc = (crc << 1);
      }
    }
  }
  return crc;
}

// Public validation function
// Wraps and builds the data type that the crc calculation private function
// works on To be called from the IR sensor task from freertos.c
bool MLX90614_VerifyData(uint8_t target_reg, volatile uint8_t *dma_rx_buffer) {
  crc_payload_t payload;

  // Populate the struct
  payload.slave_write_addr = (IR_SLAVE_ADDR << 1);
  payload.register_addr = target_reg;
  payload.slave_read_addr = (IR_SLAVE_ADDR << 1) | 0x01;
  payload.data_lsb = dma_rx_buffer[0];
  payload.data_msb = dma_rx_buffer[1];

  // Cast the struct to a uint8_t pointer so the CRC function can iterate over
  // it
  uint8_t calculated_pec =
      calculate_smbus_pec((uint8_t *)&payload, sizeof(crc_payload_t));

  // Return true if it matches the 3rd byte from the DMA buffer
  return (calculated_pec == dma_rx_buffer[2]);
}