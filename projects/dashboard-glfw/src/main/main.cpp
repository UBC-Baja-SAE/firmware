#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <iostream>
#include <cmath>
#include <string>

// Error output
void glfw_error_callback(int error, const char* description) {
    std::cerr << "GLFW Error (" << error << "): " << description << "\n";
}

int main() {
    glfwSetErrorCallback(glfw_error_callback);

    setenv("EGL_PLATFORM", "wayland", 1);
    setenv("MESA_LOADER_DRIVER_OVERRIDE", "v3d", 1);

    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    // Requests OpenGL ES 2.0
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_REFRESH_RATE, GLFW_DONT_CARE);

    // Creates the window (MAKEROBO temp display)
    GLFWwindow* window = glfwCreateWindow(800, 480, "Dashboard", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Set up the ImGui renderer backend for OpenGL ES 2.0
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 100");

    // Render Loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Start the ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Create a borderless, fullscreen ImGui window to act as a canvas
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("Gauge Canvas", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);

        // Simulate speed from 0 to 50 mph using a sine wave
        double time = glfwGetTime();
        float speed = (std::sin(time * 1.5) + 1.0f) * 25.0f; // ranges 0.0 to 50.0
        float max_speed = 50.0f;

        // ImGui drawing tools
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 center = ImVec2(io.DisplaySize.x / 2.0f, io.DisplaySize.y / 2.0f + 50.0f);
        float radius = 180.0f;

        // VFD-style colors (Bright Cyan/Green)
        ImU32 color_bg = IM_COL32(20, 50, 40, 255);       // Dim background arc
        ImU32 color_active = IM_COL32(50, 255, 200, 255); // Bright active arc and text
        ImU32 color_needle = IM_COL32(255, 50, 50, 255);  // Red needle

        // Angles in ImGui: 0 is right, PI/2 is down, PI is left, -PI/2 is up
        // Semi-circle from Left (PI) to Right (0)
        float angle_start = 3.14159f; // Left side
        float angle_end = 0.0f;       // Right side

        // Calculate the current needle angle: $\theta = \text{start} + (\text{speed} / \text{max}) \times (\text{end} - \text{start})$
        float speed_ratio = speed / max_speed;
        float angle_current = angle_start + (speed_ratio * (angle_end - angle_start));

        // Draws the background track (Dimmed)
        draw_list->PathArcTo(center, radius, angle_start, angle_end, 64);
        draw_list->PathStroke(color_bg, 0, 15.0f);

        // Draws the active speed track (Bright)
        draw_list->PathArcTo(center, radius, angle_start, angle_current, 64);
        draw_list->PathStroke(color_active, 0, 15.0f);

        // Draws the needle
        ImVec2 needle_tip = ImVec2(
            center.x + std::cos(angle_current) * (radius - 10.0f),
            center.y + std::sin(angle_current) * (radius - 10.0f)
        );
        draw_list->AddLine(center, needle_tip, color_needle, 4.0f);
        draw_list->AddCircleFilled(center, 10.0f, color_active); // Center pivot point

        // Draws the digital readout text
        char speed_text[32];
        snprintf(speed_text, sizeof(speed_text), "%.0f MPH", speed);

        // Centers the text roughly below the needle pivot
        ImVec2 text_size = ImGui::CalcTextSize(speed_text);
        ImVec2 text_pos = ImVec2(center.x - (text_size.x / 2.0f), center.y + 20.0f);
        draw_list->AddText(ImGui::GetFont(), 32.0f, text_pos, color_active, speed_text);

        ImGui::End();


        // Clears screen to pure black for maximum contrast
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Render the ImGui elements
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}