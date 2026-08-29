# H723ZGT6 FreeRTOS v10.6 template

## Tachometer Task + Speedometer Task

External Clock mode increments a counter-register using the external pulse from the hall effect sensor (Speedometer) 
and the induction sensor (Tachometer). When the task is run, the counter registers (and overflow if applicable) and the 
current time are stored. The number from this task's previous run is subtracted from the current value to calculate the
speed.

## DMA Sensor Tasks

### ADC Sensors

(E.g. Linear Potentiometers, Steering Angle, Gearbox Oil, Throttle Position)

### Communication Layer Sensors

(E.g. Strain Guage, Fuel Tank, IMU, GPS, CVT Temperature)

## CANFD Transmission Task

## Timer Channel