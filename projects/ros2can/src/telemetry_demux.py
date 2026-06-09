#!/usr/bin/env python3
import rclpy
from rclpy.node import Node

# Import the CANopen proxy message type
from canopen_interfaces.msg import COData
# Import standard ROS 2 message types for your dashboard
from std_msgs.msg import Float32, Int32

class TelemetryDemuxNode(Node):
    def __init__(self):
        super().__init__('telemetry_demuxer')

        # Subscribe to the chaotic proxy driver output
        self.subscription = self.create_subscription(
            COData,
            '/rear_ecu/rpdo',
            self.rpdo_callback,
            10) # QoS profile depth

        # Create clean, dedicated publishers for Foxglove
        self.speed_pub = self.create_publisher(Float32, '/rear_ecu/speedometer', 10)
        self.tach_pub = self.create_publisher(Float32, '/rear_ecu/tachometer', 10)

        self.get_logger().info("Rear Ecu Demuxer Started. Listening to /rear_ecu/rpdo...")

    def rpdo_callback(self, msg):
        # Index 0x2000 (Decimal 8192) -> Speedometer
        if msg.index == 8192:
            speed_msg = Float32()
            # You can add your wheel radius math here later!
            speed_msg.data = float(msg.data)
            self.speed_pub.publish(speed_msg)

        # Index 0x2001 (Decimal 8193) -> Tachometer
        elif msg.index == 8193:
            tach_msg = Float32()
            tach_msg.data = float(msg.data)
            self.tach_pub.publish(tach_msg)

def main(args=None):
    rclpy.init(args=args)
    demux_node = TelemetryDemuxNode()

    try:
        rclpy.spin(demux_node)
    except KeyboardInterrupt:
        pass

    demux_node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()