import rclpy
from rclpy.node import Node
from sensor_msgs.msg import CompressedImage
from foxglove_msgs.msg import RawAudio  # <-- Updated Schema
import cv2
import threading
import subprocess
import re

class AVLoggerNode(Node):
    def __init__(self):
        super().__init__('av_logger_node')

        self.video_pub = self.create_publisher(CompressedImage, '/image_raw/compressed', 10)
        # Using RawAudio schema
        self.audio_pub = self.create_publisher(RawAudio, '/audio/data', 10)

        self.cam_thread = threading.Thread(target=self.video_loop, daemon=True)
        self.cam_thread.start()
        self.audio_thread = threading.Thread(target=self.audio_loop, daemon=True)
        self.audio_thread.start()

    def video_loop(self):
        # ... (Your existing video_loop is perfect, keep it as is) ...
        cap = cv2.VideoCapture('/dev/video0', cv2.CAP_V4L2)
        cap.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc(*'MJPG'))
        cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
        cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
        cap.set(cv2.CAP_PROP_FPS, 15)
        while rclpy.ok() and cap.isOpened():
            ret, frame = cap.read()
            if ret:
                success, encoded_image = cv2.imencode('.jpg', frame, [int(cv2.IMWRITE_JPEG_QUALITY), 80])
                if success:
                    msg = CompressedImage()
                    msg.header.stamp = self.get_clock().now().to_msg()
                    msg.format = "jpeg"
                    msg.data = encoded_image.tobytes()
                    self.video_pub.publish(msg)

    def audio_loop(self):
        self.get_logger().info("Audio: Detecting ALSA...")
        device = "default"
        try:
            out = subprocess.check_output(['arecord', '-l']).decode('utf-8')
            match = re.search(r'card (\d+):', out)
            if match:
                device = f"plughw:{match.group(1)},0"
        except Exception:
            pass

        cmd = ['arecord', '-D', device, '-q', '-f', 'S16_LE', '-c', '1', '-r', '16000', '-t', 'raw']
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        while rclpy.ok():
            data = proc.stdout.read(2048) # Read 2048 bytes per chunk
            if data:
                msg = RawAudio()
                msg.timestamp = self.get_clock().now().to_msg()
                msg.data = data
                msg.sample_format = RawAudio.FORMAT_S16LE # Matches S16_LE
                msg.sample_rate = 16000
                msg.num_channels = 1
                try:
                    self.audio_pub.publish(msg)
                except Exception:
                    break

def main(args=None):
    rclpy.init(args=args)
    node = AVLoggerNode()
    try:
        rclpy.spin(node)
    except Exception:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()