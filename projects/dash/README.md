
# UBC Baja QT dashboard

A QT/QML application for logging CAN FD data over OpenCyphal to .mcap and streaming to foxglove

Cross compilation is possible thanks to CMake and fetch content configs for MacOS and Linux.

## Building

Toolchain and cmake settings will be written for CLion, as that was what was used for initial build and it's the main IDE we use.

### Install Dependencies for build environment

MacOS (Homebrew):
```bash
  brew install qt6
```
Windows:
```bash
  
```
Pi:
```bash
  sudo apt install qt6-base-dev qt6-declarative-dev qt6-tools-dev qt6-tools-dev-tools cmake build-essential ninja-build pkg-config libgl1-mesa-dri libgles2-mesa libdrm2 libinput10 libegl1
```


Icons used: https://www.svgrepo.com/collection/dazzle-line-icons