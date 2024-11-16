#include "serial.h"

SerialMessage createSerialMessage(uint8_t id, uint8_t data)
{
    SerialMessage msg;

    msg.start_marker = START_MARKER; // some constant
    msg.message_id = id;
    msg.message = data;
    msg.end_marker = END_MARKER; // some other constant

    return msg;
}

void sendSerialMessage(SerialMessage msg)
{
    Serial.print(msg.start_marker, HEX);
    Serial.print(msg.message_id, HEX);
    Serial.print(msg.message, HEX);
    Serial.print(msg.end_marker, HEX);
}

