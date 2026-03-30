cmake_minimum_required(VERSION 3.16)
project(pit_telemetry)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─── Protobuf ─────────────────────────────────────────────────────────────────
find_package(Protobuf REQUIRED)
include_directories(${Protobuf_INCLUDE_DIRS})

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS Core/Src/data.proto)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# ─── Foxglove SDK ─────────────────────────────────────────────────────────────
find_package(foxglove QUIET)
if(foxglove_FOUND)
    add_definitions(-DUSE_FOXGLOVE_SDK)
    message(STATUS "Foxglove SDK found")
else()
    message(WARNING "Foxglove SDK not found — WebSocket will not be available")
endif()

# ─── Sources ──────────────────────────────────────────────────────────────────
set(SOURCES
        Core/Src/main.cpp
        Core/Src/data_manager.cpp
        Core/Src/nrf24_handler.cpp
        Core/Src/mcap_logger.cpp
        ${PROTO_SRCS}
)

add_executable(pit_telemetry ${SOURCES})

target_include_directories(pit_telemetry PRIVATE
        Core/Inc
        ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(pit_telemetry PRIVATE
        ${Protobuf_LIBRARIES}
        pthread
        $<$<BOOL:${foxglove_FOUND}>:foxglove::foxglove>
)