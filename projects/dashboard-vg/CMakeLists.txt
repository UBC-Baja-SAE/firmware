cmake_minimum_required(VERSION 3.20)
project(dashboard_vg)

set(CMAKE_CXX_STANDARD 17)

# Force CMake to look in Debian's specific arm64 library folder for pkg-config files
set(ENV{PKG_CONFIG_PATH} "/usr/lib/aarch64-linux-gnu/pkgconfig:/usr/share/pkgconfig:/usr/lib/pkgconfig")

# Find pkg-config
find_package(PkgConfig REQUIRED)

# Use pkg-config to find our graphics libraries
pkg_check_modules(GLFW REQUIRED glfw3)
pkg_check_modules(THORVG REQUIRED thorvg)

# Create your executable
add_executable(dashboard_vg Core/Src/main.cpp)

# Link the libraries and include directories
target_include_directories(dashboard_vg PUBLIC ${GLFW_INCLUDE_DIRS} ${THORVG_INCLUDE_DIRS})
target_link_libraries(dashboard_vg PUBLIC ${GLFW_LIBRARIES} ${THORVG_LIBRARIES})