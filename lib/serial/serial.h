#ifndef SERIAL_H
#define SERIAL_H

#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "serial_datatypes.h"

SerialMessage createSerialMessage(uint8_t id, uint8_t data);

void sendSerialMessage(SerialMessage msg);

#endif //SERIAL_H 