cmake_minimum_required(VERSION 3.3)
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


add_executable(dashboard_qt
        Core/Src/main.cpp
        Core/Src/mainwindow.cpp
        Core/Inc/mainwindow.h
        Core/Src/mainwindow.ui
        Core/Inc/can_id.h
        Core/Src/can_messaging.cpp
        Resources/resources.qrc
        Core/Src/mcap_writer.cpp
)

target_link_libraries(dashboard_qt
        PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        mcap
)