cmake_minimum_required(VERSION 3.25)
project(minimaldash)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)

include(FetchContent)

find_package(Qt6 COMPONENTS
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

qt_add_qml_module(${PROJECT_NAME}
        URI app
        QML_FILES
        src/ui/main.qml
)

if (LINUX)
    target_compile_definitions(${PROJECT_NAME} PRIVATE LINUX)
elseif(APPLE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE APPLE)
elseif (WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::SerialBus
)