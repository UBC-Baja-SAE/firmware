cmake_minimum_required(VERSION 3.16)
project(dashboard_qt)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# --- 1. Find Core Packages ---
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets SerialPort)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets SerialPort)
find_package(PkgConfig REQUIRED)
find_package(mcap REQUIRED)
find_package(Protobuf REQUIRED)

pkg_check_modules(LZ4 REQUIRED liblz4 IMPORTED_TARGET)
pkg_check_modules(ZSTD REQUIRED libzstd IMPORTED_TARGET)

if(NOT TARGET LZ4::lz4)
    add_library(LZ4::lz4 ALIAS PkgConfig::LZ4)
endif()
if(NOT TARGET zstd::libzstd)
    add_library(zstd::libzstd ALIAS PkgConfig::ZSTD)
endif()

# --- 2. Foxglove SDK Setup ---
option(USE_FOXGLOVE_SDK "Enable Foxglove WebSocket streaming" ON)

if(USE_FOXGLOVE_SDK)
    include(FetchContent)

    # We MUST download the pre-built release zip, not the git repo.
    # The core SDK is written in Rust, so we need the pre-compiled libfoxglove.a
    FetchContent_Declare(
            foxglove
            URL https://github.com/foxglove/foxglove-sdk/releases/download/sdk%2Fv0.18.0/foxglove-v0.18.0-cpp-aarch64-unknown-linux-gnu.zip
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )

    FetchContent_MakeAvailable(foxglove)
    add_compile_definitions(USE_FOXGLOVE_SDK)

    # The ZIP file structure has include/, src/, and lib/ folders
    set(FOXGLOVE_INCLUDE_DIR "${foxglove_SOURCE_DIR}/include")

    file(GLOB FOXGLOVE_SOURCES
            "${foxglove_SOURCE_DIR}/src/*.cpp"
            "${foxglove_SOURCE_DIR}/src/server/*.cpp"
    )

    # Locate the pre-compiled Rust binary to link against
    set(FOXGLOVE_LIB "${foxglove_SOURCE_DIR}/lib/libfoxglove.a")

    message(STATUS "Foxglove SDK enabled for live WebSocket streaming.")
else()
    set(FOXGLOVE_SOURCES "")
    set(FOXGLOVE_INCLUDE_DIR "")
    set(FOXGLOVE_LIB "")
    message(STATUS "Foxglove SDK disabled. Using fallback MCAP file writer.")
endif()

# --- 3. Define Project Files ---
set(PROTO_FILES "Core/Src/data.proto")

set(SOURCES
        "Core/Src/main.cpp"
        "Core/Src/mainwindow.cpp"
        "Core/Src/can_bridge.cpp"
        "Core/Src/mcap_logger.cpp"
        "Core/Src/data_manager.cpp"
        "Core/Src/uart_handler.cpp"
        "Resources/resources.qrc"
        ${FOXGLOVE_SOURCES}
        Core/Src/nrf24_handler.cpp
)

set(HEADERS
        "Core/Inc/mainwindow.h"
        "Core/Inc/can_bridge.h"
        "Core/Inc/mcap_logger.h"
        "Core/Inc/data_manager.h"
        "Core/Inc/uart_handler.h"
        "Core/Src/mainwindow.ui"
        Core/Inc/nrf24_handler.h
)

# --- 4. Build Executable ---
add_executable(dashboard_qt
        ${PROTO_FILES}
        ${SOURCES}
        ${HEADERS}
)

# --- 5. Includes & Linking ---
target_include_directories(dashboard_qt
        PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc"
        "${CMAKE_CURRENT_BINARY_DIR}"
)

# Only add the include directory if SDK is enabled
if(USE_FOXGLOVE_SDK)
    target_include_directories(dashboard_qt PRIVATE "${FOXGLOVE_INCLUDE_DIR}")
endif()

target_link_libraries(dashboard_qt
        PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::SerialPort
        mcap
        protobuf::libprotobuf
        PkgConfig::LZ4
        PkgConfig::ZSTD
        ${FOXGLOVE_LIB}  # Links the Foxglove binary (empty if SDK is OFF)
)

protobuf_generate(TARGET dashboard_qt)