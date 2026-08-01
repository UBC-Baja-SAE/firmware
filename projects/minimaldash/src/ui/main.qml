import QtQuick
import QtQuick.Controls
import QtMultimedia

Window {
    id: root

    // Screen dimensions based on build target
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

        // ======================================
        // 1. HARDWARE-UPSCALED CLOUDS
        // ======================================
        ShaderEffect {
            id: cloudBackground
            z: -2

            // RENDER RESOLUTION: Exactly 1/4 of your 1280x480 screen
            width: 320
            height: 120

            // Stretch it out 4x visually to fill the dash
            transform: Scale {
                xScale: 4
                yScale: 4
            }

            // The shader reads these tiny dimensions to calculate correctly
            property size resolution: Qt.size(width, height)

            property real time: 0
            NumberAnimation on time {
                loops: Animation.Infinite
                from: 0; to: 10000; duration: 10000000
                running: true
            }

            // Assumes you compiled this in CMake
            fragmentShader: "qrc:/shaders/background.frag.qsb"
        }

        // ======================================
        // 2. FADING UI MASTER CONTAINER
        // ======================================
        Item {
            id: uiLayer
            anchors.fill: parent

            // 0.0 (Invisible) when idle, 1.0 (Fully visible) when logging
            opacity: true ? 1.0 : 0.0

            // Smoothly animate the fade in/out over 800 milliseconds
            Behavior on opacity {
                NumberAnimation {
                    duration: 800
                    easing.type: Easing.InOutQuad
                }
            }

            // --- A. The Dark Tint Overlay ---
            Rectangle {
                anchors.fill: parent
                z: -1
                color: Qt.rgba(0, 0, 0, 0.2) // 65% black tint to make gauges readable
            }

            // --- B. The Gauges ---
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