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
set(CMAKE_PROJECT_NAME canopen)

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

# ── CANopenNode ──────────────────────────────────────────────────────────────
set(CANOPENNODE_DIR  ${CMAKE_SOURCE_DIR}/Middlewares/CANopenSTM32/CANopenNode)
set(CANOPENSTM32_DIR ${CMAKE_SOURCE_DIR}/Middlewares/CANopenSTM32/CANopenNode_STM32)

# Core protocol stack sources (exclude the example/ subfolder)
file(GLOB CANOPENNODE_SRCS
        ${CANOPENNODE_DIR}/301/*.c
        ${CANOPENNODE_DIR}/303/*.c
        ${CANOPENNODE_DIR}/304/*.c
        ${CANOPENNODE_DIR}/305/*.c
        ${CANOPENNODE_DIR}/309/*.c
        ${CANOPENNODE_DIR}/CANopen.c
)

# STM32 HAL port
set(CANOPENSTM32_SRCS
        ${CANOPENSTM32_DIR}/CO_driver_STM32.c
        ${CANOPENSTM32_DIR}/CO_app_STM32.c
)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
        # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
        # CANopenNode
        ${CANOPENNODE_SRCS}
        ${CANOPENSTM32_SRCS}

        # Object dictionary (copied from CANopenNode/example/ and customised)
        Core/Src/OD.c
        Core/Src/CO_storageBlank.c

        # Add user sources here
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        Core/Inc
        # CANopenNode core
        ${CANOPENNODE_DIR}
        ${CANOPENNODE_DIR}/301
        ${CANOPENNODE_DIR}/303
        ${CANOPENNODE_DIR}/304
        ${CANOPENNODE_DIR}/305
        ${CANOPENNODE_DIR}/309
        # CANopenNODE STM32
        ${CANOPENSTM32_DIR}

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