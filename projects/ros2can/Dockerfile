# Native arm64 Jazzy base for Raspberry Pi
FROM ros:jazzy-ros-base

ENV DEBIAN_FRONTEND=noninteractive

# Install the specific ROS 2 CANopen components, colcon build tools, and can-utils
RUN apt-get update && apt-get install -y \
    ros-jazzy-canopen-core \
    ros-jazzy-canopen-master-driver \
    ros-jazzy-canopen-proxy-driver \
    python3-colcon-common-extensions \
    can-utils \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /ros2_ws

# Copy your source code
COPY src/ /ros2_ws/src/

# Compile the ROS 2 workspace (this generates the .bin file)
RUN /bin/bash -c "source /opt/ros/jazzy/setup.bash && colcon build --symlink-install"

COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]