# Use Debian Bookworm (matches current Raspberry Pi OS) for ARM64
FROM --platform=linux/arm64 debian:bookworm-slim

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Update and install essential build tools and dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    git \
    pkg-config \
    meson \
    ninja-build \
    # GLFW and OpenGL/Wayland/X11 dependencies
    libglfw3-dev \
    libgl1-mesa-dev \
    libegl1-mesa-dev \
    libgles2-mesa-dev \
    libxkbcommon-dev \
    wayland-protocols \
    && rm -rf /var/lib/apt/lists/*

# Set working directory for building ThorVG
WORKDIR /opt/src

# Clone, build, and install ThorVG
RUN git clone https://github.com/thorvg/thorvg.git && \
    cd thorvg && \
    meson setup build && \
    ninja -C build && \
    ninja -C build install && \
    ldconfig

# Set the default working directory for your application
WORKDIR /app

# (Optional) Verify installations
RUN pkg-config --modversion glfw3 && \
    pkg-config --modversion thorvg

# Default command
CMD ["/bin/bash"]
