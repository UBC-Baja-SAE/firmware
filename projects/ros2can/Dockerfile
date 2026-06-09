FROM ros:jazzy-ros-base

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    ros-jazzy-canopen-core \
    ros-jazzy-canopen-master-driver \
    ros-jazzy-canopen-proxy-driver \
    python3-colcon-common-extensions \
    can-utils \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /ros2_ws

COPY src/ /ros2_ws/src/

RUN /bin/bash -c "source /opt/ros/jazzy/setup.bash && colcon build"

# Generate master.bin from the DCF that cogen_dcf produced
RUN /bin/bash -c "source /opt/ros/jazzy/setup.bash && \
    /opt/ros/jazzy/bin/dcf2dev \
    /ros2_ws/install/stm32_can_bridge/share/stm32_can_bridge/config/stm32_bus/master.dcf \
    /ros2_ws/install/stm32_can_bridge/share/stm32_can_bridge/config/stm32_bus/master.bin"

COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]