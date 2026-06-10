import os
from datetime import datetime
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import ExecuteProcess, RegisterEventHandler
from launch.event_handlers import OnProcessStart

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
        respawn=True,
        respawn_delay=5.0,
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

    # 1. The custom lifecycle bringup handler we added previously
    lifecycle_bringup = RegisterEventHandler(
        OnProcessStart(
            target_action=device_container_node,
            on_start=[
                ExecuteProcess(
                    cmd=['bash', '-c',
                         'sleep 3; '
                         'for node in master rear_ecu rr_ecu rl_ecu fr_ecu fl_ecu; do '
                         '  ros2 lifecycle set /$node configure; '
                         '  ros2 lifecycle set /$node activate; '
                         'done'],
                    output='screen'
                )
            ]
        )
    )

    # 2. Add the rosbag recorder
    # '-a' records all topics. '-s mcap' forces the MCAP format.
    rosbag_recorder = ExecuteProcess(
        cmd=['ros2', 'bag', 'record', '-a', '-s', 'mcap', '-o', bag_output_path],
        output='screen'
    )

    return LaunchDescription([
        device_container_node,
        foxglove_bridge,
        lifecycle_bringup,
        rosbag_recorder     # <-- Added to the launch description
    ])