import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import ExecuteProcess, RegisterEventHandler
from launch.event_handlers import OnProcessStart

def generate_launch_description():
    pkg_dir = get_package_share_directory('stm32_can_bridge')

    bus_config_yml = '/ros2_ws/install/stm32_can_bridge/share/stm32_can_bridge/config/stm32_bus/bus.yml'
    master_config_dcf = '/ros2_ws/install/stm32_can_bridge/share/stm32_can_bridge/config/stm32_bus/master.dcf'

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

    # A non-strict automated bringup that ignores failures if a node is disconnected.
    # We trigger this whenever the device_container_node starts (or respawns).
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

    return LaunchDescription([
        device_container_node,
        foxglove_bridge,
        lifecycle_bringup
    ])