# ==========================================
# INITIALIZATION
# ==========================================
FROM debian:bookworm-slim AS builder

# Installs the heavy compilers once and caches them
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    libglfw3-dev \
    libgles2-mesa-dev \
    libegl1-mesa-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src

# Copys the raw source code
COPY CMakeLists.txt .
COPY main.cpp .

# Compiles the code
RUN cmake -B build -S . && cmake --build build


# ==========================================
# STAGE 2: DASHBOARD
# ==========================================
FROM debian:bookworm-slim

# Installs only the lightweight runtime libraries
RUN apt-get update && apt-get install -y \
    libglfw3-wayland \
    libgles2 \
    libegl1 \
    libopengl0 \
    libgl1 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Grabs the finished binary from Stage 1
COPY --from=builder /src/build/dashboard-glfw /app/dashboard-glfw

# Runs the program
CMD ["./dashboard-glfw"]
