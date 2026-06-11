import sys
import time # <-- ADDED for timeout tracking
import os
import rclpy

# --- FORCE PI 5 GRAPHICS PIPELINE ---
os.environ["QT_QPA_EGLFS_KMS_ATOMIC"] = "1"
os.environ["QT_QPA_EGLFS_INTEGRATION"] = "eglfs_kms"
os.environ["QT_QPA_EGLFS_ALWAYS_SET_MODE"] = "1"
os.environ["QT_QPA_EGLFS_FORCE888"] = "1"
os.environ["QT_QPA_EGLFS_HIDECURSOR"] = "1"
# ------------------------------------


from rclpy.node import Node
from std_msgs.msg import Float32
from sensor_msgs.msg import CompressedImage # <-- ADDED for camera tracking
from PyQt5.QtCore import QObject, pyqtSignal, pyqtProperty, QTimer, QUrl
from PyQt5.QtGui import QGuiApplication
from PyQt5.QtQml import QQmlApplicationEngine

class Backend(QObject):
    speedChanged = pyqtSignal()
    tachChanged = pyqtSignal()
    # <-- ADDED new signals
    canActiveChanged = pyqtSignal()
    camActiveChanged = pyqtSignal()

    def __init__(self):
        super().__init__()
        self._speed = 0.0
        self._tach = 0.0
        self._canActive = False
        self._camActive = False

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

    # <-- ADDED Status Properties
    @pyqtProperty(bool, notify=canActiveChanged)
    def canActive(self): return self._canActive
    @canActive.setter
    def canActive(self, val):
        if self._canActive != val:
            self._canActive = val
            self.canActiveChanged.emit()

    @pyqtProperty(bool, notify=camActiveChanged)
    def camActive(self): return self._camActive
    @camActive.setter
    def camActive(self, val):
        if self._camActive != val:
            self._camActive = val
            self.camActiveChanged.emit()

class BackendNode(Node):
    def __init__(self, backend):
        super().__init__('baja_dash_backend')
        self.backend = backend

        self.alpha = 0.35
        self.filteredSpeed = 0.0
        self.filteredTach = 0.0

        # <-- ADDED Trackers
        self.last_can_time = 0.0
        self.last_cam_time = 0.0

        self.create_subscription(Float32, '/rear_ecu/speedometer', self.speedCallback, 10)
        self.create_subscription(Float32, '/rear_ecu/tachometer', self.tachCallback, 10)
        # <-- ADDED Camera subscription (change topic if necessary)
        self.create_subscription(CompressedImage, '/image_raw/compressed', self.camCallback, 10)

        # <-- ADDED Watchdog Timer (runs every 500ms)
        self.watchdog = self.create_timer(0.5, self.checkTimeouts)

    def speedCallback(self, msg):
        self.last_can_time = time.time() # Reset CAN timeout
        self.filteredSpeed = (self.alpha * msg.data) + ((1 - self.alpha) * self.filteredSpeed)
        self.backend.speed = self.filteredSpeed

    def tachCallback(self, msg):
        self.filteredTach = (self.alpha * msg.data) + ((1 - self.alpha) * self.filteredTach)
        self.backend.tach = self.filteredTach

    def camCallback(self, msg):
        self.last_cam_time = time.time() # Reset CAM timeout

    def checkTimeouts(self):
        now = time.time()
        # If no message in the last 1.5 seconds, set status to False
        self.backend.canActive = (now - self.last_can_time) < 1.5
        self.backend.camActive = (now - self.last_cam_time) < 1.5

def main():
    rclpy.init()
    app = QGuiApplication(sys.argv)

    backend = Backend()
    rosNode = BackendNode(backend)

    # Process ROS 2 callbacks in the Qt Event Loop
    rosTimer = QTimer()
    rosTimer.timeout.connect(lambda: rclpy.spin_once(rosNode, timeout_sec=0))
    rosTimer.start(10)

    engine = QQmlApplicationEngine()
    engine.rootContext().setContextProperty("backend", backend)

    # Path inside the new Docker container
    engine.load(QUrl.fromLocalFile('/ros2_ws/gui/Main.qml'))

    if not engine.rootObjects():
        sys.exit(-1)

    exitCode = app.exec_()

    if rclpy.ok():
        rosNode.destroy_node()
        rclpy.shutdown()

    sys.exit(exitCode)

if __name__ == '__main__':
    main()