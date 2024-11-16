#include <Arduino.h>
#include "serial.h"

int myFunction(int, int);

void setup() {
  Serial.begin(9600);

  Serial.println("BEGIN SERIAL COMMUNICATION");
}

void loop() {
  SerialMessage message = createSerialMessage(0xAF, 0xFF);

  sendSerialMessage(message);

  delay(5000);
}