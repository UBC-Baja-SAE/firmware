import sys
import signal
import subprocess
import rclpy
import os
from rclpy.node import Node
from std_msgs.msg import Float32, Float64, Int32
from sensor_msgs.msg import Imu
from PyQt5.QtCore import QObject, pyqtSignal, pyqtProperty, QTimer, QUrl
from PyQt5.QtGui import QGuiApplication
from PyQt5.QtQml import QQmlApplicationEngine
from gpiozero import RotaryEncoder, Button
import threading


class TelemetryBackend(QObject):
    speedChanged = pyqtSignal()
    tachChanged = pyqtSignal()
    topicListChanged = pyqtSignal()
    selectedIndexChanged = pyqtSignal()
    menuVisibleChanged = pyqtSignal()
    scrolled = pyqtSignal(int)
    clicked = pyqtSignal()
    shutdownRequested = pyqtSignal()

    def __init__(self):
        super().__init__()
        self._speed = 0.0
        self._tach = 0.0
        self._topic_list = []
        self._selected_index = 0
        self._menu_visible = False

    @pyqtProperty(float, notify=speedChanged)
    def speed(self): return self._speed
    @speed.setter
    def speed(self, val):
        if self._speed != val:
            self._speed = val
            self.speedChanged.emit()

    @pyqtProperty(float, notify=tachChanged)
    def tach(self): return self._tach
    @tach.setter
    def tach(self, val):
        if self._tach != val:
            self._tach = val
            self.tachChanged.emit()

    @pyqtProperty(list, notify=topicListChanged)
    def topicList(self): return self._topic_list
    @topicList.setter
    def topicList(self, val):
        if self._topic_list != val:
            self._topic_list = val
            self.topicListChanged.emit()

    @pyqtProperty(int, notify=selectedIndexChanged)
    def selectedIndex(self): return self._selected_index
    @selectedIndex.setter
    def selectedIndex(self, val):
        if self._selected_index != val:
            self._selected_index = val
            self.selectedIndexChanged.emit()

    @pyqtProperty(bool, notify=menuVisibleChanged)
    def menuVisible(self): return self._menu_visible
    @menuVisible.setter
    def menuVisible(self, val):
        if self._menu_visible != val:
            self._menu_visible = val
            self.menuVisibleChanged.emit()

    def trigger_scroll(self, direction):
        self.scrolled.emit(direction)

    def trigger_click(self):
        self.clicked.emit()


SUPPORTED_TYPES = {
    'std_msgs/msg/Float32': (Float32, lambda msg: float(msg.data)),
    'std_msgs/msg/Float64': (Float64, lambda msg: float(msg.data)),
    'std_msgs/msg/Int32':   (Int32,   lambda msg: float(msg.data)),
}

IMU_FIELDS = {
    '/imu/data_raw [accel.x]': lambda msg: msg.linear_acceleration.x,
    '/imu/data_raw [accel.y]': lambda msg: msg.linear_acceleration.y,
    '/imu/data_raw [accel.z]': lambda msg: msg.linear_acceleration.z,
    '/imu/data_raw [gyro.x]':  lambda msg: msg.angular_velocity.x,
    '/imu/data_raw [gyro.y]':  lambda msg: msg.angular_velocity.y,
    '/imu/data_raw [gyro.z]':  lambda msg: msg.angular_velocity.z,
}


class TelemetryNode(Node):
    def __init__(self, backend):
        super().__init__('telemetry_display_node')
        self.backend = backend
        self.speed_alpha = 0.35
        self.filtered_speed = 0.0
        self.filtered_tach = 0.0

        self._left_sub = None
        self._left_topic = None
        self._topic_entries = []

        self.create_subscription(Float32, 'linpot', self._tach_cb, 10)
        self.create_timer(2.0, self._refresh_topics)

    def _tach_cb(self, msg):
        self.filtered_tach = (self.speed_alpha * msg.data) + ((1 - self.speed_alpha) * self.filtered_tach)
        self.backend.tach = self.filtered_tach

    def _make_left_cb(self, extractor):
        def cb(msg):
            raw = extractor(msg)
            self.filtered_speed = (self.speed_alpha * raw) + ((1 - self.speed_alpha) * self.filtered_speed)
            self.backend.speed = self.filtered_speed
        return cb

    def _refresh_topics(self):
        topic_names_and_types = self.get_topic_names_and_types()
        entries = []
        imu_active = False

        for name, types in topic_names_and_types:
            for t in types:
                if t in SUPPORTED_TYPES:
                    msg_class, extractor = SUPPORTED_TYPES[t]
                    entries.append((name, msg_class, extractor))
                    break
            if name == '/imu/data_raw':
                imu_active = True

        if imu_active:
            for label, extractor in IMU_FIELDS.items():
                entries.append((label, Imu, extractor))

        self._topic_entries = entries
        self.backend.topicList = [e[0] for e in entries]

    def subscribe_left(self, label):
        if label == self._left_topic:
            return
        if self._left_sub is not None:
            self.destroy_subscription(self._left_sub)
            self._left_sub = None

        entry = next((e for e in self._topic_entries if e[0] == label), None)
        if entry is None:
            self.get_logger().warn(f'Topic entry not found: {label}')
            return

        label, msg_class, extractor = entry
        ros_topic = '/imu/data_raw' if label.startswith('/imu/data_raw [') else label

        self._left_topic = label
        self.filtered_speed = 0.0
        self._left_sub = self.create_subscription(
            msg_class,
            ros_topic,
            self._make_left_cb(extractor),
            10
        )


class HardwareInterface:
    LONG_PRESS_SECONDS = 5

    def __init__(self, backend):
        self.backend = backend
        self.encoder = RotaryEncoder(17, 27, wrap=False, max_steps=0)
        self.button = Button(22, pull_up=True, bounce_time=0.05)

        self._long_press_timer = None
        self._long_press_fired = False

        self.encoder.when_rotated_clockwise = lambda: self.backend.trigger_scroll(1)
        self.encoder.when_rotated_counter_clockwise = lambda: self.backend.trigger_scroll(-1)

        self.button.when_pressed = self._on_press
        self.button.when_released = self._on_release

    def _on_press(self):
        self._long_press_fired = False
        self._long_press_timer = threading.Timer(
            self.LONG_PRESS_SECONDS,
            self._on_long_press
        )
        self._long_press_timer.start()

    def _on_release(self):
        if self._long_press_timer is not None:
            self._long_press_timer.cancel()
            self._long_press_timer = None

        # Only fire click if long press didn't trigger
        if not self._long_press_fired:
            self.backend.trigger_click()

    def _on_long_press(self):
        self._long_press_fired = True
        self.backend.shutdownRequested.emit()


def main():
    rclpy.init()
    app = QGuiApplication(sys.argv)

    backend = TelemetryBackend()
    ros_node = TelemetryNode(backend)
    hw_interface = HardwareInterface(backend)

    def on_scroll(direction):
        if not backend.menuVisible:
            return
        topics = backend.topicList
        if not topics:
            return
        backend.selectedIndex = (backend.selectedIndex + direction) % len(topics)

    def on_click():
        if not backend.menuVisible:
            ros_node._refresh_topics()
            QTimer.singleShot(500, ros_node._refresh_topics)
            backend.menuVisible = True
        else:
            topics = backend.topicList
            if topics:
                ros_node.subscribe_left(topics[backend.selectedIndex])
            backend.menuVisible = False

    def on_shutdown_requested():
        print("Long press detected — shutting down container...")
        ros_node.destroy_node()
        rclpy.shutdown()
        app.quit()
        # Stop Docker for a clean exit
        os.kill(1, signal.SIGTERM)

    backend.scrolled.connect(on_scroll)
    backend.clicked.connect(on_click)
    backend.shutdownRequested.connect(on_shutdown_requested)

    timer = QTimer()
    timer.timeout.connect(lambda: rclpy.spin_once(ros_node, timeout_sec=0))
    timer.start(10)

    engine = QQmlApplicationEngine()
    engine.rootContext().setContextProperty("backend", backend)
    engine.load(QUrl.fromLocalFile('/uros_ws/gui/Main.qml'))

    if not engine.rootObjects():
        sys.exit(-1)

    exit_code = app.exec_()
    ros_node.destroy_node()
    rclpy.shutdown()
    sys.exit(exit_code)


if __name__ == '__main__':
    main()