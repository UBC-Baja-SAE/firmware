cmake_minimum_required(VERSION 3.25)
project(minimaldash)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)

include(FetchContent)

find_package(Qt6 REQUIRED COMPONENTS
        Core
        Gui
        Qml
        SerialBus
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
)

set(FOXGLOVE_SDK_VERSION "0.25.2")


if (LINUX)
    set(FOXGLOVE_URL "https://github.com/foxglove/foxglove-sdk/releases/download/sdk%2Fv${FOXGLOVE_SDK_VERSION}/foxglove-v${FOXGLOVE_SDK_VERSION}-cpp-aarch64-unknown-linux-gnu.zip")
    target_compile_definitions(${PROJECT_NAME} PRIVATE LINUX)
elseif(APPLE)
    set(FOXGLOVE_URL "https://github.com/foxglove/foxglove-sdk/releases/download/sdk%2Fv${FOXGLOVE_SDK_VERSION}/foxglove-v${FOXGLOVE_SDK_VERSION}-cpp-aarch64-apple-darwin.zip")
    target_compile_definitions(${PROJECT_NAME} PRIVATE APPLE)
elseif (WIN32)
    set(FOXGLOVE_URL "https://github.com/foxglove/foxglove-sdk/releases/download/sdk%2Fv${FOXGLOVE_SDK_VERSION}/foxglove-v${FOXGLOVE_SDK_VERSION}-cpp-x86_64-pc-windows-msvc.zip")
    target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32)
endif()

FetchContent_Declare(
        foxglove_sdk
        URL ${FOXGLOVE_URL}
)
FetchContent_MakeAvailable(foxglove_sdk)

find_package(foxglove-sdk CONFIG REQUIRED HINTS "${foxglove_sdk_SOURCE_DIR}")
foxglove_sdk_add_cpp_library(foxglove_cpp TYPE STATIC)

target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::SerialBus
        foxglove_cpp
)