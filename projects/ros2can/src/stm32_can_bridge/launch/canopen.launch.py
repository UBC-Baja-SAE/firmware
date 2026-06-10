import os
from datetime import datetime
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import ExecuteProcess

def generate_launch_description():
    pkg_dir = get_package_share_directory('stm32_can_bridge')

    bus_config_yml = '/ros2_ws/install/stm32_can_bridge/share/stm32_can_bridge/config/stm32_bus/bus.yml'
    master_config_dcf = '/ros2_ws/install/stm32_can_bridge/share/stm32_can_bridge/config/stm32_bus/master.dcf'

    # Generate a unique timestamp for the bag folder name
    timestamp = datetime.now().strftime('%Y_%m_%d_%H_%M_%S')
    bag_output_path = f'/logs/can_bus_log_{timestamp}'

    device_container_node = Node(
        package='canopen_core',
        executable='device_container_node',
        name='device_container',
        output='screen',
        # Removed respawn=True to prevent Launch exceptions during debugging
        parameters=[
            {'bus_config': bus_config_yml},
            {'master_config': master_config_dcf},
            {'can_interface_name': 'can0'},
        ]
    )

    foxglove_bridge = Node(
        package='foxglove_bridge',
        executable='foxglove_bridge',
        name='foxglove_bridge',
        parameters=[{
            'port': 8765,
            'address': '0.0.0.0',
        }]
    )

    # A smart bash bringup that polls for the nodes instead of sleeping blindly
    bringup_cmd = (
        'echo "Waiting for /master lifecycle services to be available..."; '
        'until ros2 lifecycle set /master configure; do '
        '  sleep 1; '
        'done; '
        'ros2 lifecycle set /master activate; '
        'echo "Master is active! Giving ECUs 2 seconds to finish network discovery..."; '
        'sleep 2; '
        'for node in rear_ecu rr_ecu rl_ecu fr_ecu fl_ecu; do '
        '  echo "Bringing up $node..."; '
        '  ros2 lifecycle set /$node configure; '
        '  ros2 lifecycle set /$node activate; '
        'done; '
        'echo "Lifecycle bringup script finished."'
    )

    lifecycle_bringup = ExecuteProcess(
        cmd=['bash', '-c', bringup_cmd],
        output='screen'
    )

    # The rosbag recorder
    rosbag_recorder = ExecuteProcess(
        cmd=['ros2', 'bag', 'record', '-a', '-s', 'mcap', '-o', bag_output_path],
        output='screen'
    )

    return LaunchDescription([
        device_container_node,
        foxglove_bridge,
        lifecycle_bringup,
        rosbag_recorder
    ])