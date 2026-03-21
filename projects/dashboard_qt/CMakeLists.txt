cmake_minimum_required(VERSION 3.16)
project(dashboard_qt)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets)

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

set(PROTO_FILES
        Core/Src/data.proto
        Core/Src/can_handler.cpp
        Core/Src/can_translator.cpp
        Core/Src/mcap_logger.cpp

)

add_executable(dashboard_qt
        ${PROTO_FILES}
        Resources/resources.qrc
        Core/Src/main.cpp
        Core/Src/mainwindow.cpp
        Core/Inc/mainwindow.h
        Core/Src/mainwindow.ui
        Core/Src/can_handler.cpp
        Core/Inc/can_handler.h
        Core/Src/can_translator.cpp
        Core/Inc/can_translator.h
        Core/Src/mcap_logger.cpp
        Core/Inc/mcap_logger.h
        Core/Src/data_manager.cpp
        Core/Inc/data_manager.h
)

target_include_directories(dashboard_qt
        PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(dashboard_qt
        PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        mcap
        protobuf::libprotobuf
)

protobuf_generate(TARGET dashboard_qt)