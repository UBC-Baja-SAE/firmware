#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>

int main() {
    // 1. Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    // Requests OpenGL ES 2.0
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // Requests maximum refresh rate
    glfwWindowHint(GLFW_REFRESH_RATE, GLFW_DONT_CARE);

    // Borderless window
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

    // Creates the window
    GLFWwindow* window = glfwCreateWindow(800, 480, "Color Test", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    // Binds the OpenGL context to the window
    glfwMakeContextCurrent(window);

    // Render Loop
    while (!glfwWindowShouldClose(window)) {
        // Gets the current time in seconds
        double time = glfwGetTime();

        // Calculate smooth color transitions using sine waves
        // (sin(time) goes from -1 to 1, so we normalize it to 0.0 to 1.0)
        float red   = (std::sin(time * 2.0) / 2.0f) + 0.5f;
        float green = (std::sin(time * 1.3 + 1.0) / 2.0f) + 0.5f;
        float blue  = (std::sin(time * 0.8 + 2.0) / 2.0f) + 0.5f;

        // Apply the color and clear the screen
        glClearColor(red, green, blue, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Swaps the front and back buffers to display the new frame
        glfwSwapBuffers(window);

        // Process Wayland/input events
        glfwPollEvents();
    }

    // Clean up on exit
    glfwTerminate();
    return 0;
}