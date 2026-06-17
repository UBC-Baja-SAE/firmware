
# UBC Baja QT dashboard

A QT/QML application for logging CAN FD data over OpenCyphal to .mcap and streaming to foxglove

Cross compilation is possible thanks to CMake and fetch content configs for MacOS and Linux.

## Building

Toolchain and cmake settings will be written for CLion, as that was what was used for initial build and it's the main IDE we use.

### Install Dependencies for build environment

MacOS (Homebrew):
```bash
  brew install nlohmann-json websocketpp asio qt6
```
Windows:
