FROM ros:jazzy-ros-base

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    ros-jazzy-canopen-core \
    ros-jazzy-canopen-master-driver \
    ros-jazzy-canopen-proxy-driver \
    ros-jazzy-foxglove-bridge \
    python3-colcon-common-extensions \
    can-utils \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /ros2_ws

COPY src/ /ros2_ws/src/

RUN /bin/bash -c "source /opt/ros/jazzy/setup.bash && colcon build"

COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]