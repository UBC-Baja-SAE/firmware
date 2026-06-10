import sys
import rclpy
from rclpy.node import Node
from std_msgs.msg import Float32
from PyQt5.QtCore import QObject, pyqtSignal, pyqtProperty, QTimer, QUrl
from PyQt5.QtGui import QGuiApplication
from PyQt5.QtQml import QQmlApplicationEngine

class Backend(QObject):
    speedChanged = pyqtSignal()
    tachChanged = pyqtSignal()

    def __init__(self):
        super().__init__()
        self._speed = 0.0
        self._tach = 0.0

    @pyqtProperty(float, notify=speedChanged)
    def speed(self):
        return self._speed

    @speed.setter
    def speed(self, val):
        if self._speed != val:
            self._speed = val
            self.speedChanged.emit()

    @pyqtProperty(float, notify=tachChanged)
    def tach(self):
        return self._tach

    @tach.setter
    def tach(self, val):
        if self._tach != val:
            self._tach = val
            self.tachChanged.emit()

class BackendNode(Node):
    def __init__(self, backend):
        super().__init__('baja_dash_backend')
        self.backend = backend

        # Smoothing filter for UI stability
        self.alpha = 0.35
        self.filteredSpeed = 0.0
        self.filteredTach = 0.0

        # Subscribe directly to the Demuxer's output topics
        self.create_subscription(Float32, '/rear_ecu/speedometer', self.speedCallback, 10)
        self.create_subscription(Float32, '/rear_ecu/tachometer', self.tachCallback, 10)

    def speedCallback(self, msg):
        self.filteredSpeed = (self.alpha * msg.data) + ((1 - self.alpha) * self.filteredSpeed)
        self.backend.speed = self.filteredSpeed

    def tachCallback(self, msg):
        self.filteredTach = (self.alpha * msg.data) + ((1 - self.alpha) * self.filteredTach)
        self.backend.tach = self.filteredTach

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