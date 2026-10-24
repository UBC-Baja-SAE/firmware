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
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR APPLE)
    set(FOXGLOVE_OS_TRIPLE "apple-darwin")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows" OR WIN32)
    set(FOXGLOVE_OS_TRIPLE "pc-windows-msvc")
else()
    message(FATAL_ERROR "Unsupported OS for Foxglove SDK: ${CMAKE_SYSTEM_NAME}")
endif()

set(FOXGLOVE_SDK_VERSION "0.30.2")
set(FOXGLOVE_URL "https://github.com/foxglove/foxglove-sdk/releases/download/sdk%2Fv${FOXGLOVE_SDK_VERSION}/foxglove-v${FOXGLOVE_SDK_VERSION}-cpp-${FOXGLOVE_ARCH}-${FOXGLOVE_OS_TRIPLE}.zip")

include(FetchContent)
FetchContent_Declare(
        foxglove_sdk
        URL ${FOXGLOVE_URL}
        PATCH_COMMAND ${CMAKE_COMMAND}
        -D FILE=include/foxglove/parameter.hpp
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/patch_foxglove_parameter.cmake"
)

FetchContent_MakeAvailable(foxglove_sdk)

find_package(foxglove-sdk CONFIG REQUIRED HINTS "${foxglove_sdk_SOURCE_DIR}")
foxglove_sdk_add_cpp_library(Foxglove TYPE STATIC)