import QtQuick 2.15
import QtQuick.Window 2.15

Window {
    id: root
    visible: true
    width: 480
    height: 1280
    color: "black"

    Item {
        id: landscape
        width: 1280
        height: 480
        rotation: 90
        anchors.centerIn: parent

        AnimatedImage {
            anchors.fill: parent
            source: "images/background.gif"
            fillMode: Image.PreserveAspectCrop
        }

        // --- ADDED Diagnostic Icons ---
        Row {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.margins: 25
            spacing: 20
            z: 5 // Ensure it stays on top of the background

            // CAN Bus Indicator
            Rectangle {
                width: 50; height: 30; radius: 5
                // Green if active, Red if disconnected
                color: backend.canActive ? "#2ECC71" : "#E74C3C"
                border.color: "#FFFFFF"; border.width: 2

                Text {
                    anchors.centerIn: parent
                    text: "CAN"
                    color: "white"
                    font.pixelSize: 14
                    font.bold: true
                }
            }

            // Camera Indicator
            Rectangle {
                width: 50; height: 30; radius: 5
                color: backend.camActive ? "#2ECC71" : "#E74C3C"
                border.color: "#FFFFFF"; border.width: 2

                Text {
                    anchors.centerIn: parent
                    text: "CAM"
                    color: "white"
                    font.pixelSize: 14
                    font.bold: true
                }
            }
        }
        // ------------------------------

        Row {
            anchors.centerIn: parent
            anchors.verticalCenterOffset: 20
            spacing: 100

            Gauge {
                width: 450
                height: 450
                value: backend.speed
                minValue: 0
                maxValue: 60     // Top speed
                unitText: "KM/H"
                gifSource: "images/original.gif"
            }

            Gauge {
                width: 450
                height: 450
                value: backend.tach
                minValue: 0
                maxValue: 4000   // Max engine RPM
                unitText: "RPM"
                gifSource: "images/original.gif"
            }
        }
    }
}