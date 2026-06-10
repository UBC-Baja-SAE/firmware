import rclpy
from rclpy.node import Node
from sensor_msgs.msg import CompressedImage
from audio_common_msgs.msg import AudioData
import cv2
import pyaudio
import threading
import numpy as np

class AVLoggerNode(Node):
    def __init__(self):
        super().__init__('av_logger_node')

        # Publishers
        self.video_pub = self.create_publisher(CompressedImage, '/image_raw/compressed', 10)
        self.audio_pub = self.create_publisher(AudioData, '/audio/data', 20)

        # Start Camera Thread
        self.cam_thread = threading.Thread(target=self.video_loop, daemon=True)
        self.cam_thread.start()

        # Start Audio Thread
        self.audio_thread = threading.Thread(target=self.audio_loop, daemon=True)
        self.audio_thread.start()

    def video_loop(self):
        self.get_logger().info("Starting Video Stream (MJPEG 640x480 @ 15fps)")
        cap = cv2.VideoCapture('/dev/video0', cv2.CAP_V4L2)

        # Force hardware MJPEG compression from the C270
        cap.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc(*'MJPG'))
        cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
        cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
        cap.set(cv2.CAP_PROP_FPS, 15)

        while rclpy.ok() and cap.isOpened():
            ret, frame = cap.read()
            if ret:
                # Compress to JPEG format for ROS
                success, encoded_image = cv2.imencode('.jpg', frame, [int(cv2.IMWRITE_JPEG_QUALITY), 80])
                if success:
                    msg = CompressedImage()
                    msg.header.stamp = self.get_clock().now().to_msg()
                    msg.format = "jpeg"
                    msg.data = encoded_image.tobytes()
                    self.video_pub.publish(msg)

    def audio_loop(self):
        self.get_logger().info("Starting Audio Stream (16kHz Mono)")
        p = pyaudio.PyAudio()

        try:
            stream = p.open(format=pyaudio.paInt16,
                            channels=1,
                            rate=16000,
                            input=True,
                            frames_per_buffer=1024)

            while rclpy.ok():
                # Read raw bytes from the C270 Microphone
                data = stream.read(1024, exception_on_overflow=False)
                msg = AudioData()
                msg.data = list(data)
                self.audio_pub.publish(msg)

        except Exception as e:
            self.get_logger().error(f"Audio failed: {e}")

def main(args=None):
    rclpy.init(args=args)
    node = AVLoggerNode()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()