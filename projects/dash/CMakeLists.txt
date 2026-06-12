cmake_minimum_required(VERSION 4.1)
project(dash)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)


find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        REQUIRED)

add_executable(dash
        Src/main.cpp
        third_party/libcanard/canard.c
)

target_include_directories(dash PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/libcanard
        ${CMAKE_SOURCE_DIR}/third_party/mcap
)

target_link_libraries(dash
        Qt::Core
        Qt::Gui
        Qt::Widgets
)

