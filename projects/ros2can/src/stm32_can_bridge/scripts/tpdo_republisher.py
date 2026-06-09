import rclpy
from rclpy.node import Node
from canopen_interfaces.msg import COData
from std_msgs.msg import UInt32

class TPDORepublisher(Node):
    def __init__(self):
        super().__init__('tpdo_republisher')
        self.speed_pub = self.create_publisher(UInt32, '/speedometer', 10)
        self.tach_pub  = self.create_publisher(UInt32, '/tachometer', 10)
        self.create_subscription(COData, '/rear_ecu/tpdo', self.tpdo_cb, 10)

    def tpdo_cb(self, msg):
        out = UInt32()
        out.data = msg.data
        if msg.index == 0x2000:
            self.speed_pub.publish(out)
        elif msg.index == 0x2001:
            self.tach_pub.publish(out)

def main():
    rclpy.init()
    rclpy.spin(TPDORepublisher())

if __name__ == '__main__':
    main()