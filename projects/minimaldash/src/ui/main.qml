import QtQuick
import QtQuick.Controls

Window {
    id: root

    width: IsReleaseBuild ? 480 : 1280
    height: IsReleaseBuild ? 1280 : 480
    visible: true
    color: "black"

    minimumWidth: width
    maximumWidth: width
    minimumHeight: height
    maximumHeight: height

    Shortcut {
        sequence: "Escape"
        onActivated: Qt.quit()
    }

    FontLoader {
        id: customFont
        source: "qrc:/qt/qml/app/assets/fonts/microgramma.ttf"
    }

    Item {
        id: dash
        width: 1280
        height: 480
        anchors.centerIn: parent
        rotation: IsReleaseBuild ? 90 : 0

        opacity: 0.0

        Behavior on opacity {
            NumberAnimation { duration: 5000; easing.type: Easing.InOutQuad }
        }

        Component.onCompleted: dash.opacity = 1.0

        ShaderEffect {
            anchors.fill: parent
            z: -2

            property size resolution: Qt.size(width, height)

            property real time: 0
            NumberAnimation on time {
                loops: Animation.Infinite
                from: 0; to: 10000; duration: 10000000
                running: true
            }

            fragmentShader: "qrc:/shaders/background.frag.qsb"
        }

        Item {
            id: uiLayer
            anchors.fill: parent

            opacity: Data.isLogging ? 1.0 : 0.0

            Behavior on opacity {
                NumberAnimation { duration: 800; easing.type: Easing.InOutQuad }
            }

            Rectangle {
                anchors.fill: parent
                z: -1
                color: Qt.rgba(0, 0, 0, 0.3)
            }

            Debug {
                anchors.centerIn: parent
                anchors.verticalCenterOffset: 120
            }

            Indicator {
                anchors.centerIn: parent
                anchors.verticalCenterOffset: -160
                anchors.horizontalCenterOffset: -10
            }

            Row {
                anchors.centerIn: parent
                spacing: 60

                Tachometer {
                    fontFamily: customFont.name
                    rpm: Data.rpm
                    maxRpm: 4000
                }

                Steering {
                    fontFamily: customFont.name
                    angle: Data.steeringAngle
                    maxAngle: 30
                    anchors.verticalCenter: parent.verticalCenter
                }

                Speedometer {
                    fontFamily: customFont.name
                    speed: Data.speed
                    maxSpeed: 60
                }
            }
        }
    }
}