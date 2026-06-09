cmake_minimum_required(VERSION 3.8)
project(stm32_can_bridge)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(lely_core_libraries REQUIRED)

# Generate the master.dcf and master.bin files automatically in the build folder
cogen_dcf(stm32_bus)

# Generate master.bin from master.dcf
add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/config/stm32_bus/master.bin
        COMMAND dcf2dev
        ${CMAKE_CURRENT_BINARY_DIR}/config/stm32_bus/master.dcf
        ${CMAKE_CURRENT_BINARY_DIR}/config/stm32_bus/master.bin
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/config/stm32_bus/master.dcf
)
add_custom_target(generate_master_bin ALL
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/config/stm32_bus/master.bin
)

# 1. Install the standard source directories
install(DIRECTORY
        launch
        config
        DESTINATION share/${PROJECT_NAME}
)

# 2. THE MISSING PIECE: Install the auto-generated files from the build folder
install(DIRECTORY
        ${CMAKE_CURRENT_BINARY_DIR}/config/stm32_bus
        DESTINATION share/${PROJECT_NAME}/config
)

ament_package()