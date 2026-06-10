import rclpy
from rclpy.node import Node
from sensor_msgs.msg import CompressedImage
from audio_common_msgs.msg import AudioData
from rclpy.qos import QoSProfile, QoSReliabilityPolicy, QoSHistoryPolicy
import cv2
import threading
import subprocess

class AVLoggerNode(Node):
    def __init__(self):
        super().__init__('av_logger_node')

        # Foxglove/ROS standard: High-bandwidth sensors use "Best Effort" QoS
        qos_profile = QoSProfile(
            reliability=QoSReliabilityPolicy.BEST_EFFORT,
            history=QoSHistoryPolicy.KEEP_LAST,
            depth=10
        )

        self.video_pub = self.create_publisher(CompressedImage, '/image_raw/compressed', qos_profile)
        self.audio_pub = self.create_publisher(AudioData, '/audio/data', qos_profile)

        self.cam_thread = threading.Thread(target=self.video_loop, daemon=True)
        self.cam_thread.start()

        self.audio_thread = threading.Thread(target=self.audio_loop, daemon=True)
        self.audio_thread.start()

    def video_loop(self):
        self.get_logger().info("Video Thread: Connecting to /dev/video0...")
        cap = cv2.VideoCapture('/dev/video0', cv2.CAP_V4L2)
        cap.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc(*'MJPG'))
        cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
        cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
        cap.set(cv2.CAP_PROP_FPS, 15)

        if not cap.isOpened():
            self.get_logger().error("Video Thread: FAILED to open /dev/video0!")
            return

        self.get_logger().info("Video Thread: Active! Streaming frames...")
        frame_count = 0

        while rclpy.ok() and cap.isOpened():
            ret, frame = cap.read()
            if ret:
                success, encoded_image = cv2.imencode('.jpg', frame, [int(cv2.IMWRITE_JPEG_QUALITY), 80])
                if success:
                    msg = CompressedImage()
                    msg.header.stamp = self.get_clock().now().to_msg()
                    msg.header.frame_id = "camera_link"  # Required for Foxglove UI!
                    msg.format = "rgb8; jpeg"            # Standard ROS 2 format
                    msg.data = encoded_image.tobytes()

                    try:
                        self.video_pub.publish(msg)
                        frame_count += 1
                        if frame_count % 30 == 0:
                            self.get_logger().info(f"Video Thread: Successfully published {frame_count} frames...")
                    except Exception as e:
                        if rclpy.ok():
                            self.get_logger().error(f"Video Publisher Error: {e}")
            else:
                self.get_logger().warn("Video Thread: Dropped a frame from camera.")

    def audio_loop(self):
        self.get_logger().info("Audio Thread: Connecting to ALSA...")
        cmd = ['arecord', '-q', '-f', 'S16_LE', '-c', '1', '-r', '16000', '-t', 'raw']

        try:
            proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            self.get_logger().info("Audio Thread: Active! Streaming bytes...")
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
                            self.get_logger().info(f"Audio Thread: Successfully published {audio_count} chunks...")
                    except Exception as e:
                        if rclpy.ok():
                            self.get_logger().error(f"Audio Publisher Error: {e}")
                else:
                    err = proc.stderr.read().decode('utf-8')
                    if rclpy.ok():
                        self.get_logger().error(f"Audio Thread: arecord stopped! {err}")
                    break
        except Exception as e:
            self.get_logger().error(f"Audio Thread failed to start: {e}")

def main(args=None):
    rclpy.init(args=args)
    node = AVLoggerNode()
    try:
        rclpy.spin(node)
    except (KeyboardInterrupt, rclpy.executors.ExternalShutdownException):
        pass
    finally:
        if rclpy.ok():
            node.destroy_node()
            rclpy.shutdown()

if __name__ == '__main__':
    main()