import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import TimerAction, ExecuteProcess

def generate_launch_description():
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
            # Give the container more time before it declares boot failed
            {'boot_timeout': 30},
        ]
    )

    foxglove_bridge = Node(
        package='foxglove_bridge',
        executable='foxglove_bridge',
        name='foxglove_bridge',
        respawn=True,
        respawn_delay=3.0,
        parameters=[{
            'port': 8765,
            'address': '0.0.0.0',
        }]
    )

    return LaunchDescription([device_container_node, foxglove_bridge])