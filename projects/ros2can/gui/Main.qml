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