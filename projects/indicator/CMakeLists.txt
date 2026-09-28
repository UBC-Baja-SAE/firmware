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
set(CMAKE_PROJECT_NAME indicator)

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

# Add Lib Paths
set(LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../lib")
set(LVGL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/lvgl")

# Add LVGL source files dynamically
file(GLOB_RECURSE LVGL_SOURCES
        "${LVGL_DIR}/lvgl/src/*.c"
)

# Add DBC Files Dynamically
file(GLOB DBC_SOURCES
        "${LIB_DIR}/dbc/*.c"
)

# Append DBC includes to generated cmake target
target_include_directories(stm32cubemx INTERFACE
        ${LIB_DIR}/dbc
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
        # Append DBC database sources dynamically
        ${DBC_SOURCES}
        # Append LVGL sources
        ${LVGL_SOURCES}
        # Append Display Drivers
        Core/Src/lv_port_disp.c
        Core/Src/GC9A01.c
        # Add user sources here
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        ${LIB_DIR}/dbc
        ${LVGL_DIR}
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