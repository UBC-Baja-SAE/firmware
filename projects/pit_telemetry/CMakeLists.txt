cmake_minimum_required(VERSION 3.16)
project(pit_telemetry)

set(CMAKE_CXX_STANDARD 20)

# ─── Protobuf & MCAP ──────────────────────────────────────────────────────────
find_package(Protobuf REQUIRED)

# ─── Foxglove SDK ─────────────────────────────────────────────────────────────
option(USE_FOXGLOVE_SDK "Enable Foxglove WebSocket streaming" ON)

if(USE_FOXGLOVE_SDK)
    include(FetchContent)

    FetchContent_Declare(
            foxglove
            URL https://github.com/foxglove/foxglove-sdk/releases/download/sdk%2Fv0.18.0/foxglove-v0.18.0-cpp-aarch64-unknown-linux-gnu.zip
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )

    FetchContent_MakeAvailable(foxglove)
    add_compile_definitions(USE_FOXGLOVE_SDK)

    set(FOXGLOVE_INCLUDE_DIR "${foxglove_SOURCE_DIR}/include")

    file(GLOB FOXGLOVE_SOURCES
            "${foxglove_SOURCE_DIR}/src/*.cpp"
            "${foxglove_SOURCE_DIR}/src/server/*.cpp"
    )

    set(FOXGLOVE_LIB "${foxglove_SOURCE_DIR}/lib/libfoxglove.a")

    message(STATUS "Foxglove SDK enabled")
else()
    set(FOXGLOVE_SOURCES "")
    set(FOXGLOVE_INCLUDE_DIR "")
    set(FOXGLOVE_LIB "")
    message(STATUS "Foxglove SDK disabled")
endif()

# ─── Sources ──────────────────────────────────────────────────────────────────
set(PROTO_FILES "Core/Src/data.proto")

set(SOURCES
        Core/Src/main.cpp
        Core/Src/data_manager.cpp
        Core/Src/nrf24_handler.cpp
        Core/Src/mcap_logger.cpp
        ${FOXGLOVE_SOURCES}
)

# ─── Executable ───────────────────────────────────────────────────────────────
add_executable(pit_telemetry
        ${PROTO_FILES}
        ${SOURCES}
)

# ─── Includes & Linking ───────────────────────────────────────────────────────
target_include_directories(pit_telemetry PRIVATE
        Core/Inc
        ${CMAKE_CURRENT_BINARY_DIR}
)

if(USE_FOXGLOVE_SDK)
    target_include_directories(pit_telemetry PRIVATE ${FOXGLOVE_INCLUDE_DIR})
endif()

target_link_libraries(pit_telemetry PRIVATE
        protobuf::libprotobuf
        pthread
        gpiod
        ${FOXGLOVE_LIB}
)

protobuf_generate(TARGET pit_telemetry)