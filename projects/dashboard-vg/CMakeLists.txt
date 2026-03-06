cmake_minimum_required(VERSION 3.20)
project(dashboard_vg)

set(CMAKE_CXX_STANDARD 17)

# Force CMake to look in Debian's specific arm64 library folder
set(ENV{PKG_CONFIG_PATH} "/usr/lib/aarch64-linux-gnu/pkgconfig:/usr/share/pkgconfig:/usr/lib/pkgconfig")

find_package(PkgConfig REQUIRED)

# 1. Add 'gl' to your pkg_check_modules call
pkg_check_modules(GLFW REQUIRED glfw3)
pkg_check_modules(THORVG REQUIRED thorvg)
pkg_check_modules(OPENGL REQUIRED gl) # This finds the libGL symbols like glClear

add_executable(dashboard_vg Core/Src/main.cpp)

# 2. Add the OpenGL include and library variables to the target
target_include_directories(dashboard_vg PUBLIC
        ${GLFW_INCLUDE_DIRS}
        ${THORVG_INCLUDE_DIRS}
        ${OPENGL_INCLUDE_DIRS}
)

target_link_libraries(dashboard_vg PUBLIC
        ${GLFW_LIBRARIES}
        ${THORVG_LIBRARIES}
        ${OPENGL_LIBRARIES} # This resolves the 'undefined reference to glClear'
)