cmake_minimum_required(VERSION 3.25)
project(minimaldash)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Default to Debug if no build type is specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
endif()

include(FetchContent)

find_package(Qt6 REQUIRED COMPONENTS
        Core
        Gui
        Qml
        SerialBus
        Multimedia
)

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
        src/core/dash.h
        src/peripherals/webcam.cpp
        src/peripherals/webcam.h
)

file(GLOB DBC_FILES "assets/dbc/*.dbc")

qt_add_resources(${PROJECT_NAME} "dbc_resources"
        PREFIX "/"
        BASE "assets/dbc"
        FILES ${DBC_FILES}
)

qt_add_qml_module(${PROJECT_NAME}
        URI app
        QML_FILES
        src/ui/main.qml
        src/ui/Speedometer.qml
        src/ui/Tachometer.qml
        src/ui/Indicator.qml
        src/ui/Debug.qml
        RESOURCES
        "assets/eglfs/eglfs.json"
        "assets/icons/logo.png"
        "assets/fonts/microgramma.ttf"
)

# ---------------------------------------------------------
# Build Profiles: Debug vs. Release
# ---------------------------------------------------------

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Configuring for RELEASE (Linux ARM64 / EGLFS)")

    # Hardcode variables for Foxglove SDK FetchContent to guarantee
    # we pull the ARM64 Linux build during cross-compilation.
    set(FOXGLOVE_ARCH "aarch64")
    set(FOXGLOVE_OS_TRIPLE "unknown-linux-gnu")

    target_compile_definitions(${PROJECT_NAME} PRIVATE
            ENV_RELEASE
            LINUX
    )

    target_compile_options(${PROJECT_NAME} PRIVATE
            -O3
            -march=armv8-a
    )

else()
    message(STATUS "Configuring for DEBUG (Any Architecture)")

    set(TARGET_ARCH ${CMAKE_SYSTEM_PROCESSOR})
    if(TARGET_ARCH MATCHES "^(x86_64|AMD64|amd64)$")
        set(FOXGLOVE_ARCH "x86_64")
    elseif(TARGET_ARCH MATCHES "^(aarch64|arm64|ARM64)$")
        set(FOXGLOVE_ARCH "aarch64")
    else()
        message(FATAL_ERROR "Unsupported architecture for Foxglove SDK: ${TARGET_ARCH}")
    endif()

    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(FOXGLOVE_OS_TRIPLE "unknown-linux-gnu")
        target_compile_definitions(${PROJECT_NAME} PRIVATE LINUX)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR APPLE)
        set(FOXGLOVE_OS_TRIPLE "apple-darwin")
        target_compile_definitions(${PROJECT_NAME} PRIVATE APPLE)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows" OR WIN32)
        set(FOXGLOVE_OS_TRIPLE "pc-windows-msvc")
        target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32)
    else()
        message(FATAL_ERROR "Unsupported OS for Foxglove SDK: ${CMAKE_SYSTEM_NAME}")
    endif()

    target_compile_definitions(${PROJECT_NAME} PRIVATE
            ENV_DEBUG
    )

    target_compile_options(${PROJECT_NAME} PRIVATE
            -O0
            -g
            -Wall
            -Wextra
    )
endif()

# ---------------------------------------------------------
# Foxglove SDK Fetching & Linking
# ---------------------------------------------------------

set(FOXGLOVE_SDK_VERSION "0.25.2")
set(FOXGLOVE_URL "https://github.com/foxglove/foxglove-sdk/releases/download/sdk%2Fv${FOXGLOVE_SDK_VERSION}/foxglove-v${FOXGLOVE_SDK_VERSION}-cpp-${FOXGLOVE_ARCH}-${FOXGLOVE_OS_TRIPLE}.zip")

FetchContent_Declare(
        foxglove_sdk
        URL ${FOXGLOVE_URL}
        PATCH_COMMAND ${CMAKE_COMMAND}
        -DFILE=<SOURCE_DIR>/include/foxglove/parameter.hpp
        -P ${CMAKE_SOURCE_DIR}/cmake/patch_foxglove_parameter.cmake
)

FetchContent_MakeAvailable(foxglove_sdk)

find_package(foxglove-sdk CONFIG REQUIRED HINTS "${foxglove_sdk_SOURCE_DIR}")
foxglove_sdk_add_cpp_library(foxglove_cpp TYPE STATIC)

target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::SerialBus
        Qt6::Multimedia
        foxglove_cpp
)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows" OR WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ntdll)
endif()