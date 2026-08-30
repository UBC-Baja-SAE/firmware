# H723ZGT6 FreeRTOS v10.6 template

## Disclaimer
This is a test version where we are aiming to build out the current implementation of the rear ECU within a 
RTOS environment. 

## Tachometer Task + Speedometer Task

External Clock mode increments a counter-register using the external pulse from the hall effect sensor (Speedometer) 
and the induction sensor (Tachometer). When the task is run, the counter registers (and overflow if applicable) and the 
current time are stored. The number from this task's previous run is subtracted from the current value to calculate the
speed.

## DMA Sensor Tasks

H723ZGT6 has 4 DMA controllers (varying types) with enough channels to allow all sensors to use DMA. For more 
information on the controllers go to the H723 product manual. DMA will allow sensor data to update directly to
memory without needing extra CPU cycles for polling.

### ADC Sensors

(E.g. Linear Potentiometers, Steering Angle, Gearbox Oil, Throttle Position)

### Communication Layer Sensors

(E.g. Strain Guage, Fuel Tank, IMU, GPS, CVT Temperature)

## CANFD

### Transmit Task

The transmission task will be set to "ready" every 100ms arbitrated by the Timer Channel. It takes a FIFO queue 
with the sensor data and metadata. It packs the queue into a defined static CAN frame according to the DBC file.

### Frame packing
With CANFD, we have a maximum of 64 bytes / 512 bits. For robust packing, we are defining a static payload structure.
Looking at the data we aim to implement, a single frame should encompass all sensor integration. For reference, we have 
roughly calculated payload size here: [frame_packing.txt](frame_packing.txt)

## Timer Channel

Timer channel is configured to "unlock" the sensor tasks' software FIFO queue that feeds into the CAN Transmission task.
This should unlock every 100 ms, as to not overload the CAN bus.

TIM2 is selected based on its 32 bit counter resolution allowing for less frequent overflow handling and thus less 
CPU cycles wasted. 

As we develop sensor implementation we are looking to increase the "unlock" rate to decrease transmission time, 
optimizing the data rate across the bus for better resolution when processing the data.