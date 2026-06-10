#!/bin/bash
set -e

# Source the core ROS 2 environment
source /opt/ros/jazzy/setup.bash
source /ros2_ws/install/setup.bash

# Check if a command was passed to the container
if [ "$1" != "" ]; then
    # Execute the passed command instead of launching the node
    exec "$@"
else
    # Default behavior: run the launch file
    echo "Starting ProxyDriver Pub/Sub Bridge..."
    exec ros2 launch stm32_can_bridge canopen.launch.py
fi