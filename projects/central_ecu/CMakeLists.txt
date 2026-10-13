cmake_minimum_required(VERSION 3.25)
project(central_ecu LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

find_package(Eigen3 REQUIRED NO_MODULE)
find_package(Chrono REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

# Allow Debug builds to link against a Release-only Chrono library
if(CHRONO_TARGETS)
    set_target_properties(${CHRONO_TARGETS} PROPERTIES MAP_IMPORTED_CONFIG_DEBUG Release)
endif()

qt_add_executable(${PROJECT_NAME}
        src/main.cpp
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

qt_add_qml_module(${PROJECT_NAME}
        URI app
        VERSION 1.0
        QML_FILES
        src/ui/main.qml
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        ${CHRONO_TARGETS}
        Eigen3::Eigen
        Qt6::Quick
)