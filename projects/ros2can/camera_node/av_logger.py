import rclpy
from rclpy.node import Node
from sensor_msgs.msg import CompressedImage
from audio_common_msgs.msg import AudioData
import cv2
import threading
import subprocess

class AVLoggerNode(Node):
    def __init__(self):
        super().__init__('av_logger_node')
        self.video_pub = self.create_publisher(CompressedImage, '/image_raw/compressed', 10)
        self.audio_pub = self.create_publisher(AudioData, '/audio/data', 20)

        self.cam_thread = threading.Thread(target=self.video_loop, daemon=True)
        self.cam_thread.start()

        self.audio_thread = threading.Thread(target=self.audio_loop, daemon=True)
        self.audio_thread.start()

    def video_loop(self):
        self.get_logger().info("Starting Video Stream (MJPEG 640x480 @ 15fps)")
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
                    try:
                        self.video_pub.publish(msg)
                    except Exception:
                        break  # Fails cleanly if the node is shutting down

    def audio_loop(self):
        self.get_logger().info("Starting Audio Stream via ALSA arecord (16kHz Mono)")
        cmd = ['arecord', '-q', '-f', 'S16_LE', '-c', '1', '-r', '16000', '-t', 'raw']

        try:
            proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
            while rclpy.ok():
                data = proc.stdout.read(1024)
                if data:
                    msg = AudioData()
                    msg.data = list(data)
                    try:
                        self.audio_pub.publish(msg)
                    except Exception:
                        break  # Fails cleanly if the node is shutting down
        except Exception:
            pass

def main(args=None):
    rclpy.init(args=args)
    node = AVLoggerNode()
    try:
        rclpy.spin(node)
    except (KeyboardInterrupt, rclpy.executors.ExternalShutdownException):
        pass  # Catch the Ctrl+C quietly
    finally:
        if rclpy.ok():
            node.destroy_node()
            rclpy.shutdown()

if __name__ == '__main__':
    main()