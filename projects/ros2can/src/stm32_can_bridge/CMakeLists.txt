cmake_minimum_required(VERSION 3.8)
project(stm32_can_bridge)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(lely_core_libraries REQUIRED)

# Generate the master.dcf and master.bin files automatically
cogen_dcf(stm32_bus)

# Install the standard launch and config directories
install(DIRECTORY
        launch
        config
        DESTINATION share/${PROJECT_NAME}
)

ament_package()