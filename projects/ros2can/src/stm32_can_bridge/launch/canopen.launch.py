import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    pkg_dir = get_package_share_directory('stm32_can_bridge')

    # Define ALL three file paths
    bus_config_yml = os.path.join(pkg_dir, 'config', 'stm32_bus', 'bus.yml')
    master_config_dcf = os.path.join(pkg_dir, 'config', 'stm32_bus', 'master.dcf')
    master_config_bin = os.path.join(pkg_dir, 'config', 'stm32_bus', 'master.bin') # <-- Added this back

    device_container_node = Node(
        package='canopen_core',
        executable='device_container_node',
        name='device_container',
        output='screen',
        parameters=[
            {'bus_config': bus_config_yml},
            {'master_config': master_config_dcf},
            {'master_bin': master_config_bin}, # <-- Added the missing comma here
            {'can_interface_name': 'vcan0'}
        ]
    )

    return LaunchDescription([device_container_node])