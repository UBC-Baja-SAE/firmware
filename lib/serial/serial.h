/**
 * @file serial.h
 * @brief Provides an API to send serial messages from device to device.
 */

#ifndef SERIAL_H
#define SERIAL_H

#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "serial_datatypes.h"

/**
 * @brief Create a serial message to be sent
 * 
 * @param id The identifier for the message to be sent
 * @param data The data to be transmitted within the serial message
 * @return the SerialMessage object that will be transmitted
 */
SerialMessage createSerialMessage(uint8_t id, uint8_t data);

/**
 * @brief Sends a message through the serial port
 * 
 * @param msg The serial message to be sent
 */
void sendSerialMessage(SerialMessage msg);

#endif //SERIAL_H 