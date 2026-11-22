<img height="60" alt="logo" align="right" src="https://github.com/user-attachments/assets/4ff369fe-0a6e-43cc-8d50-4a43564a78e6" />

# UBC Baja SAE Firmware


## Overview

This repository contains firmware which will be used for the mechatronics systems for our vehicle for the 2026-2027 season.

## Projects

### Dashboard (minimaldash)

A Cross-compatible application for parsing CAN frames into Foxglove

### ESUS (esus_dbc)

STM32 firmware for parsing linear potentiometers and IMUs

### Front ECU (front_ecu)

STM32 firmware for parsing steering angle and button inputs

### Rear ECU (rear_dbc)

STM32 firmware for parsing a speedometer, tachometer, and IMU

### Steering Wheel (bluepill_wheel)

STM32 firmware for parsing button inputs (wip)

## DBC Compiler (tools/dbcgen)

Generates the `.c`/`.h` files in `lib/dbc` from each `.dbc`. The output is the regular cantools code plus a struct for every signal group, so each sensor task can fill its own struct and the CAN task can pack everything into one frame.

After editing a DBC, regenerate and commit the `.dbc` with the generated files:

```bash
pip install -r tools/dbcgen/requirements.txt
python tools/dbcgen/dbcgen.py
```

### Adding a sensor

1. Add the sensor's signals to the message.
2. Group them with one line: `SIG_GROUP_ <message id> <sensor name> 1 : <signal> <signal> ...;`
3. Regenerate. You get `struct <db>_<message>_<sensor>_t` and `<db>_<message>_<sensor>_to_msg()`.

In a message with groups, every signal has to be in exactly one group. Message lengths have to be a valid CAN/CAN FD size (0-8, 12, 16, 20, 24, 32, 48 or 64 bytes).

### Using groups

The group structs have no locking built in. Guard each shared instance however you like, for example with a mutex per sensor:

```c
static struct roxy_rear_daq_imu_t imu_shared;
static SemaphoreHandle_t imu_mutex; // xSemaphoreCreateMutex() before the scheduler starts

// IMU task
xSemaphoreTake(imu_mutex, portMAX_DELAY);
imu_shared = reading;
xSemaphoreGive(imu_mutex);

// CAN TX task
struct roxy_rear_daq_t msg;
uint8_t frame[ROXY_REAR_DAQ_LENGTH];

roxy_rear_daq_init(&msg);
xSemaphoreTake(imu_mutex, portMAX_DELAY);
roxy_rear_daq_imu_to_msg(&imu_shared, &msg);
xSemaphoreGive(imu_mutex);
// ...same for the other groups...
roxy_rear_daq_pack(frame, &msg, sizeof(frame));
```

Fields hold raw bus values. If a sensor gives physical units, convert with the generated `_encode()` function, e.g. `roxy_rear_daq_obj_temp_encode(25.0)`.

Frames longer than 8 bytes need FDCAN in FD mode (`FDCAN_FRAME_FD_NO_BRS` or `FDCAN_FRAME_FD_BRS`), and the TX header needs `FDFormat = FDCAN_FD_CAN`.
