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
                maxValue: 60
                unitText: "CM"
                gifSource: "images/original.gif"
                noSignal: backend.topicList.length === 0
            }

            Gauge {
                width: 450
                height: 450
                value: backend.tach
                minValue: 0
                maxValue: 60
                unitText: "CM"
                gifSource: "images/original.gif"
            }
        }

        // Topic selector overlay
        Rectangle {
            id: menuOverlay
            visible: backend.menuVisible
            anchors.centerIn: parent
            width: 500
            height: Math.min(backend.topicList.length * 60 + 40, 400)
            color: "#cc000000"
            radius: 16

            Column {
                anchors.centerIn: parent
                width: parent.width - 20
                spacing: 4

                Text {
                    text: "Select Left Gauge Topic"
                    color: "white"
                    font.pixelSize: 16
                    font.bold: true
                    anchors.horizontalCenter: parent.horizontalCenter
                    bottomPadding: 8
                }

                Repeater {
                    model: backend.topicList
                    delegate: Rectangle {
                        width: parent.width
                        height: 52
                        radius: 8
                        color: index === backend.selectedIndex ? "#F19EF9" : "transparent"

                        Text {
                            anchors.centerIn: parent
                            text: modelData
                            color: index === backend.selectedIndex ? "white" : "#aaa"
                            font.pixelSize: 20
                            font.bold: index === backend.selectedIndex
                        }
                    }
                }

                Text {
                    visible: backend.topicList.length === 0
                    text: "No Float32 topics found"
                    color: "#ff4444"
                    font.pixelSize: 18
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
        }
    }
}
