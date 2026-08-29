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
set(CMAKE_PROJECT_NAME rear_rtos)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# ==============================================================================
# HARDWARE CONFIGURATION (STM32H723xx - Cortex-M7)
# ==============================================================================
# -mthumb: Forces Thumb instruction set (required for Cortex-M)
# -mcpu: Defines the exact core
# -mfloat-abi / -mfpu: Enables the hardware floating point unit (Double Precision)
set(MCU_FLAGS "-mcpu=cortex-m7 -mthumb -mfloat-abi=hard -mfpu=fpv5-d16")

# Inject flags globally so the stm32cubemx subdirectory inherits them
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MCU_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MCU_FLAGS}")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} ${MCU_FLAGS}")

# Tell the linker to discard unused functions/data to save flash space
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${MCU_FLAGS} -Wl,--gc-sections")
# ==============================================================================

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Append SystemView includes to generated cmake
target_include_directories(stm32cubemx INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/Third_Party/SystemView/Config
        ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/Third_Party/SystemView/SEGGER
        ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/Third_Party/SystemView/SYSVIEW
        ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/Third_Party/SystemView/Sample/FreeRTOSV10.4
        ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/Third_Party/SystemView/Sample/FreeRTOSV10.4/Config/Cortex-M
)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Append SystemView sources
    Middlewares/Third_Party/SystemView/SEGGER/SEGGER_RTT.c
    Middlewares/Third_Party/SystemView/SEGGER/SEGGER_RTT_ASM_ARMv7M.S
    Middlewares/Third_Party/SystemView/SYSVIEW/SEGGER_SYSVIEW.c
    Middlewares/Third_Party/SystemView/Sample/FreeRTOSV10.4/SEGGER_SYSVIEW_FreeRTOS.c
    Middlewares/Third_Party/SystemView/Sample/FreeRTOSV10.4/Config/Cortex-M/SEGGER_SYSVIEW_Config_FreeRTOS.c
    # Add user sources here
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
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

set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/STM32H723xG_flash.ld")

target_link_options(${CMAKE_PROJECT_NAME} PRIVATE
        -T${LINKER_SCRIPT}                 # Points GCC to the linker script
        -Wl,-Map=${CMAKE_PROJECT_NAME}.map # Generates a memory map file for debugging
        --specs=nano.specs                 # Uses newlib-nano (smaller standard library footprint)
        --specs=nosys.specs                # Provides stub implementations for system calls (prevents _sbrk / _exit errors)
)
