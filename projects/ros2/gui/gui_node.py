import sys
import rclpy
from rclpy.node import Node
from std_msgs.msg import Float32
from PyQt5.QtCore import QObject, pyqtSignal, pyqtProperty, QTimer, QUrl, pyqtSlot
from PyQt5.QtGui import QGuiApplication
from PyQt5.QtQml import QQmlApplicationEngine
from gpiozero import RotaryEncoder, Button


class TelemetryBackend(QObject):
    speedChanged = pyqtSignal()
    tachChanged = pyqtSignal()
    topicListChanged = pyqtSignal()
    selectedIndexChanged = pyqtSignal()
    menuVisibleChanged = pyqtSignal()
    scrolled = pyqtSignal(int)
    clicked = pyqtSignal()

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


class TelemetryNode(Node):
    def __init__(self, backend):
        super().__init__('telemetry_display_node')
        self.backend = backend
        self.speed_alpha = 0.35
        self.filtered_speed = 0.0
        self.filtered_tach = 0.0

        # Active left gauge subscription
        self._left_sub = None
        self._left_topic = None

        # Static right gauge on linpot
        self.create_subscription(Float32, 'linpot', self._tach_cb, 10)

        # Topic discovery timer - refresh every 2 seconds
        self.create_timer(2.0, self._refresh_topics)

    def _tach_cb(self, msg):
        self.filtered_tach = (self.speed_alpha * msg.data) + ((1 - self.speed_alpha) * self.filtered_tach)
        self.backend.tach = self.filtered_tach

    def _left_cb(self, msg):
        self.filtered_speed = (self.speed_alpha * msg.data) + ((1 - self.speed_alpha) * self.filtered_speed)
        self.backend.speed = self.filtered_speed

    def _refresh_topics(self):
        # Get all active Float32 topics
        topic_names_and_types = self.get_topic_names_and_types()
        float_topics = [
            name for name, types in topic_names_and_types
            if 'std_msgs/msg/Float32' in types
        ]
        self.backend.topicList = float_topics

    def subscribe_left(self, topic):
        if topic == self._left_topic:
            return
        if self._left_sub is not None:
            self.destroy_subscription(self._left_sub)
        self._left_topic = topic
        self._left_sub = self.create_subscription(Float32, topic, self._left_cb, 10)
        self.filtered_speed = 0.0


class HardwareInterface:
    def __init__(self, backend):
        self.backend = backend
        self.encoder = RotaryEncoder(17, 27, wrap=False, max_steps=0)
        self.button = Button(22, pull_up=True, bounce_time=0.05)
        self.encoder.when_rotated_clockwise = lambda: self.backend.trigger_scroll(1)
        self.encoder.when_rotated_counter_clockwise = lambda: self.backend.trigger_scroll(-1)
        self.button.when_pressed = self.backend.trigger_click


def main():
    rclpy.init()
    app = QGuiApplication(sys.argv)

    backend = TelemetryBackend()
    ros_node = TelemetryNode(backend)
    hw_interface = HardwareInterface(backend)

    # Wire encoder scroll to index movement
    def on_scroll(direction):
        if not backend.menuVisible:
            return
        topics = backend.topicList
        if not topics:
            return
        new_idx = (backend.selectedIndex + direction) % len(topics)
        backend.selectedIndex = new_idx

    # Wire button click to open/confirm menu
    def on_click():
        if not backend.menuVisible:
            # Open menu, refresh topics first
            ros_node._refresh_topics()
            backend.menuVisible = True
        else:
            # Confirm selection
            topics = backend.topicList
            if topics:
                selected = topics[backend.selectedIndex]
                ros_node.subscribe_left(selected)
            backend.menuVisible = False

    backend.scrolled.connect(on_scroll)
    backend.clicked.connect(on_click)

    timer = QTimer()
    timer.timeout.connect(lambda: rclpy.spin_once(ros_node, timeout_sec=0))
    timer.start(10)

    engine = QQmlApplicationEngine()
    engine.rootContext().setContextProperty("backend", backend)
    engine.load(QUrl.fromLocalFile('/uros_ws/gui/speedometer.qml'))

    if not engine.rootObjects():
        sys.exit(-1)

    exit_code = app.exec_()
    ros_node.destroy_node()
    rclpy.shutdown()
    sys.exit(exit_code)


if __name__ == '__main__':
    main()
