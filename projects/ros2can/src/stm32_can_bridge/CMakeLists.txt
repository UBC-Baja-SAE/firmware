cmake_minimum_required(VERSION 3.8)
project(stm32_can_bridge)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(lely_core_libraries REQUIRED)

cogen_dcf(stm32_bus)

# Generate master.bin from master.dcf
# cogen_dcf creates a target named after the argument, depend on it explicitly
add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/config/stm32_bus/master.bin
        COMMAND /opt/ros/jazzy/bin/dcf2dev
        ${CMAKE_CURRENT_BINARY_DIR}/config/stm32_bus/master.dcf
        ${CMAKE_CURRENT_BINARY_DIR}/config/stm32_bus/master.bin
        DEPENDS stm32_bus  # <-- depend on the cogen_dcf target, not the file
)
add_custom_target(generate_master_bin ALL
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/config/stm32_bus/master.bin
)

install(DIRECTORY
        launch
        config
        DESTINATION share/${PROJECT_NAME}
)

install(DIRECTORY
        ${CMAKE_CURRENT_BINARY_DIR}/config/stm32_bus
        DESTINATION share/${PROJECT_NAME}/config
)

ament_package()