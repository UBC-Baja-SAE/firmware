cmake_minimum_required(VERSION 3.25)
project(central_ecu LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

include(cmake/Foxglove.cmake)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick SerialBus)

qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

qt_add_executable(${PROJECT_NAME}
        src/main.cpp
        src/core/cansocket.cpp
        src/core/cansocket.h
        src/core/dbcparser.cpp
        src/core/dbcparser.h
        src/core/foxglove.cpp
        src/core/foxglove.h
        src/core/guibridge.cpp
        src/core/guibridge.h
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
        CONFIG_DIR="${CMAKE_CURRENT_SOURCE_DIR}/config")

qt_add_qml_module(${PROJECT_NAME}
        URI app
        VERSION 1.0
        QML_FILES
        src/ui/main.qml
)

file(GLOB_RECURSE ASSETS
        CONFIGURE_DEPENDS
        "src/assets/*"
)

qt_add_resources(${PROJECT_NAME} "assets"
        PREFIX "/"
        BASE "src/assets"
        FILES
        ${ASSETS}
)

get_filename_component(DBC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../lib/dbc" ABSOLUTE)

qt_add_resources(${PROJECT_NAME} "dbc"
        PREFIX "/"
        BASE "${DBC_DIR}"
        FILES
        "${DBC_DIR}/roxy.dbc"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt6::Quick
        Qt6::SerialBus
        Foxglove
        ntdll
)