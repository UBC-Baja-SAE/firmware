import QtQuick
import QtQuick.Controls
import QtQuick.Layouts


ApplicationWindow {
    id: root
    visible: true
    width: 1280
    height: 480
    title: "Test"
    color: "#193664"

    // Auto-fullscreen only on Pi
    visibility: Qt.platform.os === "linux" ? Window.FullScreen : Window.Windowed

    AnimatedImage {
        id: backgroundAnimation
        source: "assets/images/background.gif"
        anchors.fill: parent
        playing: true
    }

    FontLoader {
        id: customFont
        source: "assets/fonts/FOT-NewRodin Pro EB.otf"
    }

}