#include <iostream>
#include <vector>
#include <atomic>
#include <thread>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include <GLFW/glfw3.h>
#include <thorvg.h>

std::atomic<bool> toggle_color{false};

void uart_listener() {
    int serial_port = open("/dev/ttyS0", O_RDONLY);
    if (serial_port < 0) return;

    struct termios tty;
    tcgetattr(serial_port, &tty);
    cfsetispeed(&tty, B115200);
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_lflag &= ~(ICANON | ECHO | ISIG);
    tcsetattr(serial_port, TCSANOW, &tty);

    char c;
    std::string message_buffer = "";

    while (true) {
        if (read(serial_port, &c, 1) > 0) {
            if (c == '\n') {
                // We found the end of the line!
                std::cout << "Complete Message Received: " << message_buffer << std::endl;

                // Now we only toggle ONCE per message
                toggle_color = !toggle_color;

                // Clear the buffer for the next message
                message_buffer = "";
            } else {
                // Keep building the string
                message_buffer += c;
            }
        }
    }
}

int main() {
    std::thread(uart_listener).detach();

    if (tvg::Initializer::init(0) != tvg::Result::Success) return -1;
    if (!glfwInit()) return -1;

    int width = 800, height = 480;
    GLFWwindow* window = glfwCreateWindow(width, height, "Baja Dash", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    // Use ARGB8888 as it's the most common for Pi software buffers
    std::vector<uint32_t> buffer(width * height);
    auto canvas = tvg::SwCanvas::gen();
    canvas->target(buffer.data(), width, width, height, tvg::ColorSpace::ARGB8888);

    auto rect = tvg::Shape::gen(); // rect is already a Shape* (raw pointer)
    rect->appendRect(200, 100, 400, 280, 30, 30);
    rect->fill(255, 0, 0, 255);   // Start red
    canvas->add(std::move(rect));  // Canvas takes ownership

    // In render loop


    bool last_state = false;

    while (!glfwWindowShouldClose(window)) {
        // 3. Update logic (UART)
        if (toggle_color != last_state) {
            last_state = toggle_color.load();
            if (last_state) rect->fill(0, 255, 200, 255); // Teal
            else rect->fill(255, 0, 0, 255);             // Red
        }

        // 4. Manual Background (Clear the buffer)
        std::fill(buffer.begin(), buffer.end(), 0xFF1A1A1A);

        // 5. FORCE update and draw every frame to stop the flashing
        canvas->update();
        if (canvas->draw() == tvg::Result::Success) {
            canvas->sync();
        }

        // 6. Push to screen
        glRasterPos2i(-1, -1);
        glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, buffer.data());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    tvg::Initializer::term();
    return 0;
}