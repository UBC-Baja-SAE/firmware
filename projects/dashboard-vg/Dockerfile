# syntax=docker/dockerfile:1
FROM debian:bookworm-slim

ENV DEBIAN_FRONTEND=noninteractive

# Let Debian manage its native pkg-config paths for ARM64 multiarch
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    rm -f /etc/apt/apt.conf.d/docker-clean && \
    apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    build-essential \
    pkg-config \
    meson \
    ninja-build \
    libglfw3-wayland \
    libglfw3-dev \
    libgl1-mesa-dev \
    libegl1-mesa-dev \
    libgles2-mesa-dev \
    libxkbcommon-dev \
    wayland-protocols

WORKDIR /opt/src

COPY thorvg ./thorvg

# Build, install, and use a wildcard to find the versioned .pc file
RUN cd thorvg && \
    meson setup build --prefix=/usr && \
    ninja -C build && \
    ninja -C build install && \
    ldconfig && \
    PC_PATH=$(find /usr -name "thorvg*.pc" | head -n 1) && \
    mkdir -p /usr/share/pkgconfig && \
    ln -sf "$PC_PATH" /usr/share/pkgconfig/thorvg.pc

WORKDIR /app

# Verify installations
RUN pkg-config --modversion glfw3 && \
    pkg-config --modversion thorvg

CMD ["/bin/bash"]
