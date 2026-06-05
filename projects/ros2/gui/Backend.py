import sys
import signal
import os
import rclpy
import threading
from rclpy.node import Node
from std_msgs.msg import Float32, Float64, Int32
from PyQt5.QtCore import QObject, pyqtSignal, pyqtProperty, QTimer, QUrl
from PyQt5.QtGui import QGuiApplication
from PyQt5.QtQml import QQmlApplicationEngine
from gpiozero import RotaryEncoder, Button


class Backend(QObject):
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
        self._topicList = []
        self._selectedIndex = 0
        self._menuVisible = False

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
    def topicList(self): return self._topicList
    @topicList.setter
    def topicList(self, val):
        if self._topicList != val:
            self._topicList = val
            self.topicListChanged.emit()

    @pyqtProperty(int, notify=selectedIndexChanged)
    def selectedIndex(self): return self._selectedIndex
    @selectedIndex.setter
    def selectedIndex(self, val):
        if self._selectedIndex != val:
            self._selectedIndex = val
            self.selectedIndexChanged.emit()

    @pyqtProperty(bool, notify=menuVisibleChanged)
    def menuVisible(self): return self._menuVisible
    @menuVisible.setter
    def menuVisible(self, val):
        if self._menuVisible != val:
            self._menuVisible = val
            self.menuVisibleChanged.emit()

    def triggerScroll(self, direction):
        self.scrolled.emit(direction)

    def triggerClick(self):
        self.clicked.emit()


# Mapping of supported ROS message types to their class and value extractor
supportedTypes = {
    'std_msgs/msg/Float32': (Float32, lambda msg: float(msg.data)),
    'std_msgs/msg/Float64': (Float64, lambda msg: float(msg.data)),
    'std_msgs/msg/Int32':   (Int32,   lambda msg: float(msg.data)),
}


class BackendNode(Node):
    def __init__(self, backend):
        super().__init__('backend_display_node')
        self.backend = backend
        self.speedAlpha = 0.35
        self.filteredSpeed = 0.0
        self.filteredTach = 0.0

        self.leftSub = None
        self.leftTopic = None
        self.topicEntries = []

        self.create_subscription(Float32, 'tachometer', self.tachCallback, 10)
        self.create_timer(2.0, self.refreshTopics)

    def tachCallback(self, msg):
        self.filteredTach = (self.speedAlpha * msg.data) + ((1 - self.speedAlpha) * self.filteredTach)
        self.backend.tach = self.filteredTach

    def makeLeftCallback(self, extractor):
        def cb(msg):
            raw = extractor(msg)
            self.filteredSpeed = (self.speedAlpha * raw) + ((1 - self.speedAlpha) * self.filteredSpeed)
            self.backend.speed = self.filteredSpeed
        return cb

    def refreshTopics(self):
        topicNamesAndTypes = self.get_topic_names_and_types()
        entries = []

        for name, types in topicNamesAndTypes:
            for t in types:
                if t in supportedTypes:
                    msgClass, extractor = supportedTypes[t]
                    entries.append((name, msgClass, extractor))
                    break

        self.topicEntries = entries
        self.backend.topicList = [e[0] for e in entries]

    def subscribeLeft(self, label):
        if label == self.leftTopic:
            return

        if self.leftSub is not None:
            self.destroy_subscription(self.leftSub)
            self.leftSub = None

        entry = next((e for e in self.topicEntries if e[0] == label), None)
        if entry is None:
            self.get_logger().warn(f'Topic entry not found: {label}')
            return

        topicName, msgClass, extractor = entry

        self.leftTopic = topicName
        self.filteredSpeed = 0.0

        self.leftSub = self.create_subscription(
            msgClass,
            topicName,
            self.makeLeftCallback(extractor),
            10
        )


class HardwareInterface:
    longPressSeconds = 5

    def __init__(self, backend):
        self.backend = backend
        self.encoder = RotaryEncoder(17, 27, wrap=False, max_steps=0)
        self.button = Button(22, pull_up=True, bounce_time=0.05)

        self.longPressTimer = None
        self.longPressFired = False

        self.encoder.when_rotated_clockwise = lambda: self.backend.triggerScroll(1)
        self.encoder.when_rotated_counter_clockwise = lambda: self.backend.triggerScroll(-1)

        self.button.when_pressed = self.onPress
        self.button.when_released = self.onRelease

    def onPress(self):
        self.longPressFired = False
        self.longPressTimer = threading.Timer(
            self.longPressSeconds,
            self.onLongPress
        )
        self.longPressTimer.start()

    def onRelease(self):
        if self.longPressTimer is not None:
            self.longPressTimer.cancel()
            self.longPressTimer = None

        if not self.longPressFired:
            self.backend.triggerClick()

    def onLongPress(self):
        self.longPressFired = True
        self.backend.shutdownRequested.emit()


def main():
    rclpy.init()
    app = QGuiApplication(sys.argv)

    backend = Backend()
    rosNode = BackendNode(backend)
    hwInterface = HardwareInterface(backend)

    def onScroll(direction):
        if not backend.menuVisible:
            return
        topics = backend.topicList
        if not topics:
            return
        backend.selectedIndex = (backend.selectedIndex + direction) % len(topics)

    def onClick():
        if not backend.menuVisible:
            rosNode.refreshTopics()
            QTimer.singleShot(500, rosNode.refreshTopics)
            backend.menuVisible = True
        else:
            topics = backend.topicList
            if topics:
                rosNode.subscribeLeft(topics[backend.selectedIndex])
            backend.menuVisible = False

    def onShutdownRequested():
        print("Long press detected — stopping rosbag to unblock container shutdown...")
        app.quit()

        # Kill the ros2 bag process directly instead of signalling Bash
        os.system("pkill -SIGINT -f 'ros2 bag'")

    backend.scrolled.connect(onScroll)
    backend.clicked.connect(onClick)
    backend.shutdownRequested.connect(onShutdownRequested)

    timer = QTimer()
    timer.timeout.connect(lambda: rclpy.spin_once(rosNode, timeout_sec=0))
    timer.start(10)

    engine = QQmlApplicationEngine()
    engine.rootContext().setContextProperty("backend", backend)
    engine.load(QUrl.fromLocalFile('/uros_ws/gui/Main.qml'))

    if not engine.rootObjects():
        sys.exit(-1)

    exitCode = app.exec_()
    # Safely clean up ROS only if it's still running
    if rclpy.ok():
        rosNode.destroy_node()
        rclpy.shutdown()

    sys.exit(exitCode)


if __name__ == '__main__':
    main()