cmake_minimum_required(VERSION 3.20)

project(dashboard-glfw)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)

pkg_check_modules(GLFW REQUIRED IMPORTED_TARGET glfw3)

find_package(OpenGL REQUIRED)

add_executable(dashboard-glfw main.cpp)

target_link_libraries(dashboard-glfw PRIVATE PkgConfig::GLFW OpenGL::GL)