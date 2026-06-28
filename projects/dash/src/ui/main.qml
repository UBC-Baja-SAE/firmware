import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia
import Qt.labs.folderlistmodel

ApplicationWindow {
    id: root
    visible: true

    readonly property bool isLinux: Qt.platform.os === "linux"
    width: isLinux ? 480 : 1280
    height: isLinux ? 1280 : 480

    minimumWidth: width
    maximumWidth: width
    minimumHeight: height
    maximumHeight: height

    property color bgblue: "#193664"
    property color tabActiveColor: "#03101e"

    title: "Dash"
    color: bgblue

    visibility: isLinux ? Window.FullScreen : Window.Windowed

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

        function onRawFrameReceived(time, canId, data) {
            canSniffer.updateFrame(time, canId, data)
        }
    }

    Shortcut {
        sequence: "Left"
        onActivated: {
            if (swipeView.currentIndex > 0)
                swipeView.currentIndex--
        }
    }

    Shortcut {
        sequence: "Right"
        onActivated: {
            if (swipeView.currentIndex < swipeView.count - 1)
                swipeView.currentIndex++
        }
    }

    Shortcut {
        sequence: "Escape"
        context: Qt.ApplicationShortcut
        onActivated: {
            console.log("Quit requested via keyboard.");
            Qt.quit();
        }
    }

    FontLoader {
        id: customFont
        source: "assets/fonts/FOT-NewRodin Pro EB.otf"
    }

    Item {
        id: rotatedContentCanvas
        width: 1280
        height: 480

        rotation: root.isLinux ? 90 : 0
        transformOrigin: Item.TopLeft
        x: root.isLinux ? 480 : 0
        y: 0

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

            MediaPlayer {
                id: mediaPlayer
                audioOutput: AudioOutput {
                    id: audioOut
                    volume: 1
                    Component.onCompleted: console.log("Audio device:", audioOut.device.description, "| id:", audioOut.device.id)
                }
                onErrorOccurred: (error, errorString) => console.log("MediaPlayer error:", error, errorString)
                onHasAudioChanged: console.log("hasAudio:", hasAudio)
                onPlaybackStateChanged: console.log("playbackState:", playbackState)
                onMediaStatusChanged: {
                    console.log("mediaStatus:", mediaStatus)
                    if (mediaStatus === MediaPlayer.EndOfMedia) {
                        music.playNext();
                    }
                }
            }

            Item {
                id: canSniffer

                function updateFrame(time, canId, data) {
                    frameModel.insert(0, { "time": time, "canId": canId, "data": data })
                    if (frameModel.count > 100) {
                        frameModel.remove(100, 1)
                    }
                }

                Rectangle {
                    anchors.centerIn: parent
                    anchors.verticalCenterOffset: -10
                    width: parent.width * 0.8
                    height: parent.height * 0.7
                    radius: 12
                    color: Qt.rgba(root.tabActiveColor.r, root.tabActiveColor.g, root.tabActiveColor.b, 0.4)

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 20
                        spacing: 10

                        Text {
                            text: "CAN0 Monitor"
                            color: "white"
                            font.family: customFont.name
                            font.pointSize: 16
                            Layout.alignment: Qt.AlignHCenter
                        }


                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "Time"; color: "white"; font.bold: true; font.family: "monospace"; Layout.preferredWidth: 100 }
                            Text { text: "Id"; color: "white"; font.bold: true; font.family: "monospace"; Layout.preferredWidth: 80 }
                            Text { text: "Payload"; color: "white"; font.bold: true; font.family: "monospace"; Layout.fillWidth: true }
                        }

                        ListView {
                            id: snifferList
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            model: ListModel { id: frameModel }

                            delegate: RowLayout {
                                width: snifferList.width
                                height: implicitHeight

                                Text { text: model.time; color: "lightgray"; font.family: "monospace"; font.pointSize: 14; Layout.preferredWidth: 100 }
                                Text { text: model.canId; color: "#4facf7"; font.family: "monospace"; font.pointSize: 14; font.bold: true; Layout.preferredWidth: 80 }
                                Text { text: model.data; color: "white"; font.family: "monospace"; font.pointSize: 14; Layout.fillWidth: true }
                            }
                        }
                    }
                }
            }

            Item {
                id: cam

                Rectangle {
                    anchors.centerIn: parent
                    anchors.verticalCenterOffset: -10
                    width: parent.width * 0.8
                    height: parent.height * 0.7
                    radius: 12
                    color: Qt.rgba(root.tabActiveColor.r, root.tabActiveColor.g, root.tabActiveColor.b, 0.4)
                    clip: true

                    VideoOutput {
                        objectName: "dashVideoOutput"
                        anchors.fill: parent
                        anchors.margins: 2
                        fillMode: VideoOutput.PreserveAspectCrop
                    }

                    Text {
                        anchors.top: parent.top
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.margins: 15
                        text: "Dashcam"
                        color: "white"
                        font.family: customFont.name
                        font.pointSize: 16
                    }
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
                radius: 12
                color: Qt.rgba(root.tabActiveColor.r, root.tabActiveColor.g, root.tabActiveColor.b, 0.2)

                Row {
                    id: navRow
                    anchors.centerIn: parent
                    spacing: 40

                    Repeater {
                        model: [
                            { icon: "assets/icons/gauge.svg", index: 0 },
                            { icon: "assets/icons/music.svg", index: 1 },
                            { icon: "assets/icons/map.svg", index: 2 },
                            { icon: "assets/icons/video.svg", index: 3 }
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
}