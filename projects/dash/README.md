
# UBC Baja QT dashboard

A QT/QML application for logging CAN FD data over OpenCyphal to .mcap and streaming to foxglove

Cross compilation is possible thanks to CMake and fetch content configs for MacOS and Linux.

## Building

### Install Dependencies for build environment

MacOS (Homebrew):
```bash
brew install cmake ninja qt
```
Windows:

Install Qt6 with Qt maintenance tool and pray (no good package managers for windows)

Pi (or ARM64 linux):
```bash
sudo apt update
sudo apt install -y \
  build-essential cmake ninja-build pkg-config \
  qt6-base-dev qt6-declarative-dev qt6-multimedia-dev qt6-serialbus-dev \
  qml6-module-qtquick-controls qml6-module-qtquick-layouts qml6-module-qtmultimedia \
  libgl1-mesa-dev \
  gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-libav
```

### CMake config for Pi

Setup pi as remote host, then add as environment variable:
```bash
QT_QPA_PLATFORM="eglfs QT_QPA_EGLFS_INTEGRATION=eglfs_kms QT_QPA_EGLFS_KMS_CONFIG=/home/ubcbaja/eglfs.json"
```

### CMake config for Windows

Use Visual Studio (MSVC) toolchain rather than MinGW and add the following as environment variables:
```bash
-DCMAKE_PREFIX_PATH=<LOCAL QT PATH HERE> (eg: C:\Qt\6.11.1\msvc2022_64)
-DCMAKE_C_FLAGS="-DCANARD_64_BIT=1"
-DCMAKE_CXX_FLAGS="-DCANARD_64_BIT=1"
```
Foxglove currently has an issue building on windows in parameter.hpp (cmake-build-/_deps/foxglove_sdk-src/include/foxglove/parameter.hpp)
to fix, add #ifndef FOXGLOVE_HIDE_TEMPLATES to line 466 and #endif at line 522
this needs to be done everytime cmake build directory is reset/fetch content pulls foxglove sdk

Icons used: https://www.svgrepo.com/collection/dazzle-line-icons at 1.5 stroke and FFFFFF stroke-color