import rclpy
from rclpy.node import Node
from sensor_msgs.msg import CompressedImage
from audio_common_msgs.msg import AudioData
import cv2
import threading
import subprocess
import re

class AVLoggerNode(Node):
    def __init__(self):
        super().__init__('av_logger_node')

        # Standard Reliable QoS (Best Effort over websockets drops frames in Foxglove)
        self.video_pub = self.create_publisher(CompressedImage, '/image_raw/compressed', 10)
        self.audio_pub = self.create_publisher(AudioData, '/audio/data', 10)

        self.cam_thread = threading.Thread(target=self.video_loop, daemon=True)
        self.cam_thread.start()

        self.audio_thread = threading.Thread(target=self.audio_loop, daemon=True)
        self.audio_thread.start()

    def video_loop(self):
        self.get_logger().info("Video: Connecting to /dev/video0...")
        cap = cv2.VideoCapture('/dev/video0', cv2.CAP_V4L2)
        cap.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc(*'MJPG'))
        cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
        cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
        cap.set(cv2.CAP_PROP_FPS, 15)

        frame_count = 0
        while rclpy.ok() and cap.isOpened():
            ret, frame = cap.read()
            if ret:
                success, encoded_image = cv2.imencode('.jpg', frame, [int(cv2.IMWRITE_JPEG_QUALITY), 80])
                if success:
                    msg = CompressedImage()
                    msg.header.stamp = self.get_clock().now().to_msg()
                    msg.header.frame_id = "camera_link"
                    msg.format = "jpeg"  # Foxglove strict format! Do not change!
                    msg.data = encoded_image.tobytes()
                    try:
                        self.video_pub.publish(msg)
                        frame_count += 1
                        if frame_count % 30 == 0:
                            self.get_logger().info(f"Video: Published {frame_count} frames")
                    except Exception:
                        break

    def audio_loop(self):
        self.get_logger().info("Audio: Detecting ALSA USB Card...")

        device = "default"
        try:
            # Ask ALSA to list all capture devices and scrape the card number
            out = subprocess.check_output(['arecord', '-l']).decode('utf-8')
            match = re.search(r'card (\d+):', out)
            if match:
                device = f"plughw:{match.group(1)},0"
                self.get_logger().info(f"Audio: Auto-detected mic at {device}")
        except Exception:
            self.get_logger().warn("Audio: Auto-detect failed, trying default")

        cmd = ['arecord', '-D', device, '-q', '-f', 'S16_LE', '-c', '1', '-r', '16000', '-t', 'raw']

        try:
            proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            audio_count = 0
            while rclpy.ok():
                data = proc.stdout.read(1024)
                if data:
                    msg = AudioData()
                    msg.data = list(data)
                    try:
                        self.audio_pub.publish(msg)
                        audio_count += 1
                        if audio_count % 100 == 0:
                            self.get_logger().info(f"Audio: Published {audio_count} chunks")
                    except Exception:
                        break
        except Exception as e:
            self.get_logger().error(f"Audio failed: {e}")

def main(args=None):
    rclpy.init(args=args)
    node = AVLoggerNode()
    try:
        rclpy.spin(node)
    except (KeyboardInterrupt, Exception):
        pass
    finally:
        if rclpy.ok():
            node.destroy_node()
            rclpy.shutdown()

if __name__ == '__main__':
    main()