# ==========================================
# INITIALIZATION
# ==========================================
FROM arm64v8/debian:bookworm-slim AS builder

# Installs the heavy compilers once and caches them
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    libglfw3-wayland \
    libglfw3-dev \
    libgles2-mesa-dev \
    libegl1-mesa-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src

# Copy the entire project context (CMakeLists.txt, src/, and imgui/)
# This preserves the exact folder structure CMake expects.
COPY . .

# Compiles the code
# Compiles the code and explicitly tells ImGui to ignore X11
RUN cmake -B build -S . -DCMAKE_CXX_FLAGS="-DIMGUI_IMPL_GLFW_DISABLE_X11" && cmake --build build

# Installs the runtime libraries AND the Mesa hardware drivers
# ==========================================
# STAGE 2: DASHBOARD
# ==========================================
FROM arm64v8/debian:bookworm-slim

# Installs the runtime libraries, Mesa drivers, and core Wayland dependencies
RUN apt-get update && apt-get install -y \
    libglfw3-wayland \
    libgles2 \
    libegl1 \
    libopengl0 \
    libgl1 \
    libgl1-mesa-dri \
    libwayland-client0 \
    libwayland-cursor0 \
    libwayland-egl1 \
    libxkbcommon0 \
    libdecor-0-0 \
    libgbm1 \
    mesa-vulkan-drivers \
    mesa-utils-extra \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Grabs the finished binary from Stage 1
COPY --from=builder /src/build/dashboard-glfw /app/dashboard-glfw

# Runs the program
CMD ["./dashboard-glfw"]