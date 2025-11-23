#ifndef STEERING_WHEEL_H
#define STEERING_WHEEL_H

#include "steering_wheel_datatypes.h"

void steering_wheel_init(SteeringWheelData_t &data, LiquidCrystal_I2C &lcd);

void steering_wheel_handler(SteeringWheelData_t &data, LiquidCrystal_I2C &lcd);

#endif // STEERING_WHEEL_H