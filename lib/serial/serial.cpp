/**
 * @file serial.c 
 * @brief Interface for communicating Serial messages
 */

#include "serial.h"

SerialMessage_t createSerialMessage(uint8_t id, uint8_t data)
{
    SerialMessage_t msg;

    msg.message_id = id;
    msg.message = data;

    return msg;
}

void sendSerialMessage(SerialMessage_t msg)
{
    Serial.print(msg.start_marker, HEX);
    Serial.print(msg.message_id, HEX);
    Serial.print(msg.message, HEX);
    Serial.print(msg.end_marker, HEX);
}

