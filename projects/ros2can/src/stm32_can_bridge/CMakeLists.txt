cmake_minimum_required(VERSION 3.8)
project(stm32_can_bridge)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(lely_core_libraries REQUIRED)

cogen_dcf(stm32_bus)

install(DIRECTORY
        launch
        config
        DESTINATION share/${PROJECT_NAME}
)

install(DIRECTORY
        ${CMAKE_CURRENT_BINARY_DIR}/config/stm32_bus
        DESTINATION share/${PROJECT_NAME}/config
)

install(PROGRAMS
        src/demuxer.py
        DESTINATION lib/${PROJECT_NAME}
)

ament_package()