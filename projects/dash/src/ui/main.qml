import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia
import Qt.labs.folderlistmodel

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

    property color bgblue: "#193664"
    property color tabActiveColor: "#03101e"

    title: "Dash"
    color: bgblue

    visibility: Qt.platform.os === "linux" ? Window.FullScreen : Window.Windowed

    property real speedometerValue: 0
    property real tachometerValue: 0

    Connections {
        target: CanAdapter
        function onUiDataUpdated(key, value) {
            if (key === "speedometer")
                speedometerValue = value
            else if (key === "tachometer")
                tachometerValue = value
        }
    }

    FontLoader {
        id: customFont
        source: "assets/fonts/FOT-NewRodin Pro EB.otf"
    }

    // Background gif fills the entire window
    AnimatedImage {
        id: backgroundAnimation
        anchors.fill: parent
        source: "assets/images/background.gif"
        playing: true
        fillMode: Image.PreserveAspectCrop
    }

    SwipeView {
        id: swipeView
        anchors.fill: parent
        orientation: Qt.Horizontal

        Item {
            id: main

            Row {
                anchors.centerIn: parent
                anchors.verticalCenterOffset: 20
                spacing: 100

                Gauge {
                    width: 450
                    height: 450
                    value: root.speedometerValue
                    minValue: 0
                    maxValue: 60
                    unitText: "KM/H"
                    gifSource: "assets/images/original.gif"
                }

                Gauge {
                    width: 450
                    height: 450
                    value: root.tachometerValue
                    minValue: 0
                    maxValue: 4000
                    unitText: "RPM"
                    gifSource: "assets/images/original.gif"
                }
            }
        }

        MusicPlayer {
            id: music
            tabActiveColor: root.tabActiveColor
            customFontName: customFont.name
        }

        Item {
            id: map

            Text {
                anchors.centerIn: parent
                font.pointSize: 25
                text: "map"
            }
        }

        Item {
            id: cam

            Text {
                anchors.centerIn: parent
                font.pointSize: 25
                text: "camera"
            }
        }
    }

    Rectangle {
        id: footerBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 70
        color: "transparent"

        Rectangle {
            id: dockBackground
            anchors.centerIn: parent
            anchors.verticalCenterOffset: -8
            width: navRow.implicitWidth + 16
            height: navRow.implicitHeight + 16
            radius: 12 // Smooth pill-like corners
            color: Qt.rgba(root.tabActiveColor.r, root.tabActiveColor.g, root.tabActiveColor.b, 0.2)

            Row {
                id: navRow
                anchors.centerIn: parent
                spacing: 40

                Repeater {
                    model: [
                        { icon: "assets/icons/gauge.svg", index: 0 },
                        { icon: "assets/icons/music.svg", index: 1 },
                        { icon: "assets/icons/video.svg", index: 2 },
                        { icon: "assets/icons/map.svg", index: 3 }
                    ]

                    delegate: Rectangle {
                        width: 50
                        height: 50
                        radius: 8
                        color: swipeView.currentIndex === modelData.index
                            ? Qt.rgba(root.tabActiveColor.r, root.tabActiveColor.g, root.tabActiveColor.b, 0.2)
                            : "transparent"

                        Image {
                            anchors.centerIn: parent
                            width: 35
                            height: 35
                            source: modelData.icon
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: swipeView.currentIndex = modelData.index
                        }
                    }
                }
            }
        }
    }
}