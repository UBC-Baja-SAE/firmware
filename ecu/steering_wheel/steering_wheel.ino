#include "steering_wheel.h"
#include "steering_wheel_datatypes.h"

//inputs from the bus

SteeringWheelData_t steering_wheel_data = {0};

void setup() {
    steering_wheel_init(steering_wheel_data);
}

void loop() {
    steering_wheel_handler(steering_wheel_data);
}