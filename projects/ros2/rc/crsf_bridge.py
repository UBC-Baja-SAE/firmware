#!/usr/bin/env python3
import argparse
import serial
import rclpy
from rclpy.node import Node
# from crsf_parser import CRSFParser # Import your parser here
# from sensor_msgs.msg import Joy    # Import ROS 2 messages here

class CRSFBridgeNode(Node):
    def __init__(self, port, baud):
        super().__init__('crsf_bridge_node')
        self.port = port
        self.baud = baud
        self.get_logger().info(f"Starting CRSF bridge on {self.port} at {self.baud} baud")
        
        # Initialize Serial
        try:
            self.ser = serial.Serial(self.port, self.baud, timeout=1)
            self.get_logger().info("Serial connection established.")
        except serial.SerialException as e:
            self.get_logger().error(f"Failed to open serial port: {e}")
            return
            
        # Initialize your CRSF Parser here
        # self.parser = CRSFParser()
        
        # Setup ROS 2 publishers/subscribers here
        # self.joy_pub = self.create_publisher(Joy, 'rc_joy', 10)
        
        # Timer to read serial data continuously
        self.timer = self.create_timer(0.01, self.read_serial)

    def read_serial(self):
        if self.ser.in_waiting > 0:
            data = self.ser.read(self.ser.in_waiting)
            # Pass data to your CRSF parser here
            # parsed_data = self.parser.parse(data)
            # if parsed_data.is_valid:
            #     self.publish_joy(parsed_data)

def main(args=None):
    parser = argparse.ArgumentParser(description='CRSF to ROS 2 Bridge')
    parser.add_argument('--port', type=str, required=True, help='Serial port (e.g., /dev/ttyAMA0)')
    parser.add_argument('--baud', type=int, required=True, help='Baud rate (e.g., 420000)')
    parsed_args, ros_args = parser.parse_known_args()

    rclpy.init(args=ros_args)
    node = CRSFBridgeNode(parsed_args.port, parsed_args.baud)
    
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()