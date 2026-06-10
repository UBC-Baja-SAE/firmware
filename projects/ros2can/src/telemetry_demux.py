#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
import struct
from functools import partial

from canopen_interfaces.msg import COData
from std_msgs.msg import Float32

class UnifiedDemuxNode(Node):
    def __init__(self):
        super().__init__('demuxer')

        # A nested dictionary to store all our publishers dynamically
        self.pubs = {}

        # ==========================================
        # 1. SETUP REAR ECU (Legacy Mapping)
        # ==========================================
        self.pubs['rear_ecu'] = {
            'speedometer': self.create_publisher(Float32, '/rear_ecu/speedometer', 10),
            'tachometer': self.create_publisher(Float32, '/rear_ecu/tachometer', 10)
        }
        self.create_subscription(COData, '/rear_ecu/rpdo', self.rear_ecu_callback, 10)

        # ==========================================
        # 2. SETUP CORNER ECUS
        # ==========================================
        self.corner_ecus = ['rr_ecu', 'rl_ecu', 'fr_ecu', 'fl_ecu']

        # Mapping dictionary: (index, subindex) -> ('topic_name', is_real32)
        self.corner_mapping = {
            (0x2000, 0x00): ('linear_potentiometer', False), # UNSIGNED32
            (0x2001, 0x01): ('accel_x', True),               # REAL32
            (0x2001, 0x02): ('accel_y', True),               # REAL32
            (0x2001, 0x03): ('accel_z', True),               # REAL32
            (0x2001, 0x04): ('gyro_x', True),                # REAL32
            (0x2001, 0x05): ('gyro_y', True),                # REAL32
            (0x2001, 0x06): ('gyro_z', True)                 # REAL32
        }

        # Create publishers and subscriptions for all 4 corner ECUs automatically
        for ecu in self.corner_ecus:
            self.pubs[ecu] = {}
            for (idx, sub), (topic, _) in self.corner_mapping.items():
                self.pubs[ecu][topic] = self.create_publisher(Float32, f'/{ecu}/{topic}', 10)

            # functools.partial lets us use ONE callback function for all 4 ECUs
            # by "baking in" the ecu parameter when creating the subscription
            self.create_subscription(
                COData,
                f'/{ecu}/rpdo',
                partial(self.corner_ecu_callback, ecu_name=ecu),
                10
            )

        self.get_logger().info("Unified Demuxer Started. Listening to all ECUs...")

    def decode_real32(self, raw_uint32):
        """Reinterprets a raw 32-bit unsigned integer as a Little-Endian IEEE 754 float."""
        # <I packs as Little-Endian Unsigned Int. <f unpacks as Little-Endian Float.
        return struct.unpack('<f', struct.pack('<I', raw_uint32 & 0xFFFFFFFF))[0]

    def rear_ecu_callback(self, msg):
        """Original routing logic for the Rear ECU."""
        # Index 0x2000 (Hex is much easier to read than decimal 8192!)
        if msg.index == 0x2000 and msg.subindex == 0x00:
            out_msg = Float32()
            out_msg.data = float(msg.data)
            self.pubs['rear_ecu']['speedometer'].publish(out_msg)

        elif msg.index == 0x2001 and msg.subindex == 0x00:
            out_msg = Float32()
            out_msg.data = float(msg.data)
            self.pubs['rear_ecu']['tachometer'].publish(out_msg)

    def corner_ecu_callback(self, msg, ecu_name):
        """Dynamic routing logic for all 4 corner ECUs."""
        # Look up the incoming index and subindex in our dictionary map
        mapping = self.corner_mapping.get((msg.index, msg.subindex))

        if mapping:
            topic_name, is_real32 = mapping
            out_msg = Float32()

            if is_real32:
                # Unmangle the raw bits into a proper floating point number
                out_msg.data = self.decode_real32(msg.data)
            else:
                # Safe to cast normally for the linear potentiometer (UNSIGNED32)
                out_msg.data = float(msg.data)

            # Publish to the specific ECU's dynamically generated publisher
            self.pubs[ecu_name][topic_name].publish(out_msg)


def main(args=None):
    rclpy.init(args=args)
    demux_node = UnifiedDemuxNode()

    try:
        rclpy.spin(demux_node)
    except KeyboardInterrupt:
        pass

    demux_node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()