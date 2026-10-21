cmake_minimum_required(VERSION 3.25)
project(central_ecu LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

qt_add_executable(${PROJECT_NAME}
        src/main.cpp
)

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


include(cmake/Foxglove.cmake)

if(CMAKE_SYSTEM_NAME STREQUAL "LINUX")
    target_compile_definitions(${PROJECT_NAME} PRIVATE LINUX)
elseif(APPLE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE APPLE)
elseif(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt6::Quick
        Foxglove
)