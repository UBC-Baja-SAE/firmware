#ifndef SERIAL_H
#define SERIAL_H

#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "serial_datatypes.h"

/**
 * Creates a `SerialMessage` instance with the associated `id` and `data` fields
 * @param   id      the value of the identifier tag associated with the message
 * @param   data    the binary value of the data being transmitted
 * @return  the `SerialMessage` that encapsulates the given `id` and `data`
 */
SerialMessage_t createSerialMessage(uint8_t id, uint64_t data);

/**
 * Sends a message over the device's serial port
 * @param   msg     the `SerialMessage` to be sent
 */
void sendSerialMessage(SerialMessage_t msg);

/**
 * Parses input data from the device's serial port into a `SerialMessage_t`
 */
SerialMessage_t readSerialMessage();

#endif //SERIAL_H 