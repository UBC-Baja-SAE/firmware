cmake_minimum_required(VERSION 3.16)
project(dashboard_qt)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets SerialPort)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets SerialPort)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LZ4 REQUIRED liblz4 IMPORTED_TARGET)
pkg_check_modules(ZSTD REQUIRED libzstd IMPORTED_TARGET)

if(NOT TARGET LZ4::lz4)
    add_library(LZ4::lz4 ALIAS PkgConfig::LZ4)
endif()
if(NOT TARGET zstd::libzstd)
    add_library(zstd::libzstd ALIAS PkgConfig::ZSTD)
endif()

find_package(mcap REQUIRED)

find_package(Protobuf REQUIRED)

# Optional: Foxglove SDK for WebSocket streaming
# Uncomment to enable Foxglove WebSocket streaming
# Download from: https://github.com/foxglove/foxglove-sdk/releases
# option(USE_FOXGLOVE_SDK "Enable Foxglove WebSocket streaming" OFF)
# if(USE_FOXGLOVE_SDK)
#     find_package(foxglove REQUIRED)
#     add_compile_definitions(USE_FOXGLOVE_SDK)
# endif()

set(PROTO_FILES
        Core/Src/data.proto
        Core/Src/can_bridge.cpp
        Core/Src/can_processor.cpp
        Core/Src/mcap_logger.cpp
        Core/Src/uart_handler.cpp

)

add_executable(dashboard_qt
        ${PROTO_FILES}
        Resources/resources.qrc
        Core/Src/main.cpp
        Core/Src/mainwindow.cpp
        Core/Inc/mainwindow.h
        Core/Src/mainwindow.ui
        Core/Src/can_bridge.cpp
        Core/Inc/can_bridge.h
        Core/Src/can_processor.cpp
        Core/Inc/can_processor.h
        Core/Src/mcap_logger.cpp
        Core/Inc/mcap_logger.h
        Core/Src/data_manager.cpp
        Core/Inc/data_manager.h
        Core/Src/uart_handler.cpp
        Core/Inc/uart_handler.h
)

target_include_directories(dashboard_qt
        PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(dashboard_qt
        PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::SerialPort
        mcap
        protobuf::libprotobuf
)

# Link Foxglove SDK if enabled
# if(USE_FOXGLOVE_SDK)
#     target_link_libraries(dashboard_qt PRIVATE foxglove::foxglove)
# endif()

protobuf_generate(TARGET dashboard_qt)