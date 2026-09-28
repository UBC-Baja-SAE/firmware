cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME rtos_h743)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
        # Add user defined library search paths
)

# Add Lib Paths
set(LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../lib")
set(SYSVIEW_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/Third_Party/SystemView")
set(DRIVER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/st7735")

# Add SystemView Files Dynamically
file(GLOB SYSVIEW_SOURCES
        "${SYSVIEW_DIR}/SEGGER/*.c"
        "${SYSVIEW_DIR}/SEGGER/*.S"
        "${SYSVIEW_DIR}/SYSVIEW/*.c"
        "${SYSVIEW_DIR}/Sample/FreeRTOSV10.4/*.c"
        "${SYSVIEW_DIR}/Sample/FreeRTOSV10.4/Config/Cortex-M/*.c"
)

# Add DBC Files Dynamically
file(GLOB DBC_SOURCES
        "${LIB_DIR}/dbc/*.c"
)

# Add SystemView includes to generated cmake
target_include_directories(stm32cubemx INTERFACE
        ${SYSVIEW_DIR}/Config
        ${SYSVIEW_DIR}/SEGGER
        ${SYSVIEW_DIR}/SYSVIEW
        ${SYSVIEW_DIR}/Sample/FreeRTOSV10.4
        ${SYSVIEW_DIR}/Sample/FreeRTOSV10.4/Config/Cortex-M
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
        # Append SystemView sources
        ${SYSVIEW_SOURCES}
        # Append Display Driver
        ${DRIVER_DIR}/st7735.c
        ${DRIVER_DIR}/fonts.c
        # Append DBC database
        ${DBC_SOURCES}
        # Add user sources here
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        ${DRIVER_DIR}
        ${LIB_DIR}/dbc
        # Add user defined include paths
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
