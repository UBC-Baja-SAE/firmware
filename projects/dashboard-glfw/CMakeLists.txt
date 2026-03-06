cmake_minimum_required(VERSION 3.20)

project(dashboard-glfw)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)

pkg_check_modules(GLFW REQUIRED IMPORTED_TARGET glfw3)

find_library(GLES2_LIBRARY GLESv2 REQUIRED)

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/imgui)

file(GLOB IMGUI_SOURCES
        ${IMGUI_DIR}/*.cpp
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

add_executable(dashboard-glfw src/main/main.cpp ${IMGUI_SOURCES})

target_include_directories(dashboard-glfw PRIVATE
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
)

target_compile_definitions(dashboard-glfw PRIVATE IMGUI_IMPL_OPENGL_ES2)

target_link_libraries(dashboard-glfw PRIVATE PkgConfig::GLFW ${GLES2_LIBRARY})