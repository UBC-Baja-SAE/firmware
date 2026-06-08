#!/bin/bash
# ros_entrypoint.sh — sources ROS overlays for every container invocation.
# Must NOT use ~/.bashrc (only runs for interactive login shells).

set -e

source /opt/ros/jazzy/setup.bash

# ros2_canopen overlay (built in Dockerfile.canopen-base)
if [ -f /canopen_ws/install/setup.bash ]; then
    source /canopen_ws/install/setup.bash
fi

# Application workspace overlay (built/mounted at runtime)
if [ -f /ros_ws/install/setup.bash ]; then
    source /ros_ws/install/setup.bash
fi

exec "$@"
