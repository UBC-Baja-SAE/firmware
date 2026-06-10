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
set(CMAKE_PROJECT_NAME esuscanopen)

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

# CANopenNode paths
set(CANOPEN_STM32_DIR ${CMAKE_SOURCE_DIR}/Middlewares/CANopenNodeSTM32/CANopenNode_STM32)
set(CANOPEN_NODE_DIR  ${CMAKE_SOURCE_DIR}/Middlewares/CANopenNodeSTM32/CANopenNode)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
        # Add user sources here

        # Object Dictionary
        Core/Src/OD.c

        # CANopenNodeSTM32
        ${CANOPEN_STM32_DIR}/CO_app_STM32.c
        ${CANOPEN_STM32_DIR}/CO_driver_STM32.c

        # CANopenNode core
        ${CANOPEN_NODE_DIR}/CANopen.c
        ${CANOPEN_NODE_DIR}/301/CO_ODinterface.c
        ${CANOPEN_NODE_DIR}/301/CO_Emergency.c
        ${CANOPEN_NODE_DIR}/301/CO_HBconsumer.c
        ${CANOPEN_NODE_DIR}/301/CO_NMT_Heartbeat.c
        ${CANOPEN_NODE_DIR}/301/CO_PDO.c
        ${CANOPEN_NODE_DIR}/301/CO_SDOclient.c
        ${CANOPEN_NODE_DIR}/301/CO_SDOserver.c
        ${CANOPEN_NODE_DIR}/301/CO_SYNC.c
        ${CANOPEN_NODE_DIR}/301/CO_TIME.c
        ${CANOPEN_NODE_DIR}/301/crc16-ccitt.c
        ${CANOPEN_NODE_DIR}/303/CO_LEDs.c
        ${CANOPEN_NODE_DIR}/305/CO_LSSmaster.c
        ${CANOPEN_NODE_DIR}/305/CO_LSSslave.c
        ${CANOPEN_NODE_DIR}/storage/CO_storage.c
        ${CANOPEN_NODE_DIR}/extra/CO_trace.c

)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        # Add user defined include paths

        # Object Dictionary
        Core/Inc

        # CANopenNodeSTM32
        ${CANOPEN_STM32_DIR}

        # CANopenNode core
        ${CANOPEN_NODE_DIR}
        ${CANOPEN_NODE_DIR}/301
        ${CANOPEN_NODE_DIR}/303
        ${CANOPEN_NODE_DIR}/305
        ${CANOPEN_NODE_DIR}/storage
        ${CANOPEN_NODE_DIR}/extra

        # OD generated files (if in a subdirectory, adjust accordingly)
        ${CMAKE_SOURCE_DIR}
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
        # Add user defined symbols
        CO_CONFIG_STORAGE=0   # disable storage if not using flash/eeprom
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
