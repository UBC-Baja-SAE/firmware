cmake_minimum_required(VERSION 3.22)
cmake_policy(SET CMP0135 NEW)
project(dash VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

include(FetchContent)

# Shared Dependencies
find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick Widgets SerialBus)

# Set QT Resource format
qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

# Download Foxglove SDK for appropriate platform
set(FOXGLOVE_SDK_VERSION "0.25.1")

if (APPLE AND CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
    message(STATUS "Fetching Foxglove SDK for macOS")
    set(FOXGLOVE_URL "https://github.com/foxglove/foxglove-sdk/releases/download/sdk%2Fv${FOXGLOVE_SDK_VERSION}/foxglove-v${FOXGLOVE_SDK_VERSION}-cpp-aarch64-apple-darwin.zip")
elseif (CMAKE_SYSTEM_NAME MATCHES "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    message(STATUS "Fetching Foxglove SDK for Linux (AArch64 / Pi 5)")
    set(FOXGLOVE_URL "https://github.com/foxglove/foxglove-sdk/releases/download/sdk%2Fv${FOXGLOVE_SDK_VERSION}/foxglove-v${FOXGLOVE_SDK_VERSION}-cpp-aarch64-unknown-linux-gnu.zip")
elseif (WIN32)
    message(STATUS "Fetching Foxglove SDK for Windows")
    set(FOXGLOVE_URL "https://github.com/foxglove/foxglove-sdk/releases/download/sdk%2Fv${FOXGLOVE_SDK_VERSION}/foxglove-v${FOXGLOVE_SDK_VERSION}-cpp-x86_64-pc-windows-msvc.zip")
endif()

FetchContent_Declare(
        foxglove_sdk
        URL ${FOXGLOVE_URL}
)
FetchContent_MakeAvailable(foxglove_sdk)

# Load Foxglove CMake config and compile C++ wrapper
find_package(foxglove-sdk CONFIG REQUIRED HINTS "${foxglove_sdk_SOURCE_DIR}")
foxglove_sdk_add_cpp_library(foxglove_cpp TYPE STATIC)

# Entrypoint
qt_add_executable(${PROJECT_NAME}
        src/main.cpp
        src/core/can_socket.h
)

#EGLFS config
qt_add_resources(${PROJECT_NAME} "app_config"
        PREFIX "/"
        BASE "src/config"
        FILES
        src/config/eglfs.json
)

# QML files
qt_add_qml_module(${PROJECT_NAME}
        URI Roxy
        VERSION 1.0
        QML_FILES
            src/ui/main.qml
            src/ui/Gauge.qml
        RESOURCES
            "src/ui/assets/images/background.gif"
            "src/ui/assets/images/original.gif"
            "src/ui/assets/fonts/FOT-NewRodin Pro EB.otf"

)



# Cross Compilation for Mac or Linux
if (APPLE OR WIN32)
    message(STATUS "Configuring for Desktop Build")
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEVELOPMENT)
    target_sources(${PROJECT_NAME} PRIVATE src/core/can_dev.cpp)

elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
    message(STATUS "Configuring for Linux Build")
    target_compile_definitions(${PROJECT_NAME} PRIVATE RELEASE)
    target_sources(${PROJECT_NAME} PRIVATE src/core/can_socket.cpp)
endif()

# Link Libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::Widgets
        Qt6::SerialBus
        foxglove_cpp
)