import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    visible: true
    width: 1280
    height: 480

    // Locks window to fixed 1280x480
    minimumWidth: 1280
    maximumWidth: 1280
    minimumHeight: 480
    maximumHeight: 480

    title: "Test"
    color: "#193664"

    visibility: Qt.platform.os === "linux" ? Window.FullScreen : Window.Windowed

    Item {
        id: content
        width: 1280
        height: 480
        anchors.centerIn: parent
        scale: Math.min(root.width / width, root.height / height)
        transformOrigin: Item.Center

        AnimatedImage {
            id: backgroundAnimation
            source: "assets/images/background.gif"
            anchors.fill: parent
            playing: true
            fillMode: Image.PreserveAspectCrop
        }

        FontLoader {
            id: customFont
            source: "assets/fonts/FOT-NewRodin Pro EB.otf"
        }

        // --- Gauges ---
        Row {
            anchors.centerIn: parent
            anchors.verticalCenterOffset: 20
            spacing: 100

            Gauge {
                width: 450
                height: 450
                value: 32 // Static test value
                minValue: 0
                maxValue: 60
                unitText: "KM/H"
                gifSource: "assets/images/original.gif"
            }

            Gauge {
                width: 450
                height: 450
                value: 2500 // Static test value
                minValue: 0
                maxValue: 4000
                unitText: "RPM"
                gifSource: "assets/images/original.gif"
            }
        }
    }
}