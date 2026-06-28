# Minimal Qt Dashboard

A simple cross-compatible application for parsing CAN frames into Foxglove

## Features
- Parses DBC files into JSON foxglove topics
- Records all topics to .mcap
- Displays all topics on a websocket `ws://0.0.0.0:8765`
- GUI customization with QML
- SocketCAN support for linux deployment
- VirtualCAN support for debugging

## Installation

Install required dependencies for your platform

Debian/Ubuntu:
```
sudo apt install qt6-{base,serialbus}-dev
```
Fedora:
```
sudo dnf install qt6-{base,serialbus}*devel*
```
MacOS (Homebrew):
```
brew install qt6 qtserialbus
```

Windows devices (and/or MacOS devices without Homebrew) can instead use the [Qt Maintenance Tool](https://doc.qt.io/qt-6/get-and-install-qt.html):

- Qt 6.11.1
  - MSVC 2022 64-bit / ARM64 (based on system architecture, Windows only)
  - Additional Libraries
    - Qt Serial Bus
## Building


## Usage

##

