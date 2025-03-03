# Electronic Control Units (ECUs)

###### Author: Donghwa Kim

## Overview

This module contains the source code for the firmware programmed onto our
automotive electronic control units (ECUs). Each subdirectory in this module
contains source code for a unique ECU.

## Requirements

The microcontroller units (MCUs) on the ECUs are in the [STM32](https://www.st.com/en/microcontrollers-microprocessors/stm32-32-bit-arm-cortex-mcus.html)
family. Hence, many of the tools used to develop for these MCUs are created directly by ST Microelectronics.

### Programming

Aside from the PCBs that physically contain the STM32 MCUs, the following tools
are recommended to program the firmware onto the MCUs:

- The [ST-Link/V2 debugger/programmer](https://www.st.com/en/development-tools/st-link-v2.html) for debugging and programming the MCUs.
  - Refer to the [user manual](https://www.st.com/resource/en/user_manual/um1075-stlinkv2-incircuit-debuggerprogrammer-for-stm8-and-stm32-stmicroelectronics.pdf) for the pinout.
- The [STM32 Cube IDE](https://www.st.com/en/development-tools/stm32cubeide.html)
  for generating the hardware abstraction layer (HAL) code, configuring the microcontroller clock speeds and pin configurations, and flashing the firmware onto the hardware.
- The [PCAN-USB](https://www.peak-system.com/PCAN-USB.199.0.html?L=1) from PEAK-System. This provides an interface between the CAN bus and the connected computer, enabling analysis and testing.
  - Refer to the [user manual](https://www.peak-system.com/produktcd/Pdf/English/PCAN-USB_UserMan_eng.pdf) for the pinout.
- The [PCAN-View](https://www.peak-system.com/PCAN-View.242.0.html?L=1) software from PEAK-System for viewing and sending CAN bus messages. This is a free software designed to work with the PCAN-USB.

## Development

### Configuration

Before writing firmware for the devices, the STM32 microcontrollers must first be configured with the correct clock speeds, CAN bus bit timing, and GPIO pin assignments so that the Cube IDE can auto-generate valid HAL source code. 

#### CAN bus bit timing

As the CAN bus is designed to accommodate 1 Mbps, the baud rate of the microcontrollers must match that.

[CAN bus bit timing configuration](https://community.st.com/t5/stm32-mcus-products/stm32f103c8t6-blue-pill-can-does-not-work/td-p/66829)



