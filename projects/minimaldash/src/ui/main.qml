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

        // 1. Put all your gauges into a single container
        Item {
            id: uiContainer
            anchors.fill: parent

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

        // 2. Capture the UI container as a live texture, and hide the original
        ShaderEffectSource {
            id: uiTexture
            sourceItem: uiContainer
            hideSource: true
            live: true
        }

        // 3. The Shader Background (now handles both background AND drawing the UI)
        ShaderEffect {
            anchors.fill: parent

            property size resolution: Qt.size(width, height)
            property variant uiSource: uiTexture
            property real uiDepth: Data.isLogging ? 0.0 : 2.8

            // 1. Cloud Density maps RPM (0 to 4000) into 0.0 to 1.0
            property real cloudDensity: Data.rpm / 4000.0

            // 2. Time of Day maps Speed (0 to 60) into 0.0 (Sunset) to 1.0 (Midday)
            property real timeOfDay: Math.min(1.0, Data.speed / 60.0)

            // 3. Pan Offset reverses the steering direction (-30 to 30) into -1.0 to 1.0
            property real panOffset: -(Data.steeringAngle / 30.0)

            // Smoothly ease the panning motion so it feels like real camera inertia
            Behavior on panOffset {
                NumberAnimation {
                    duration: 200
                    easing.type: Easing.OutSine
                }
            }

            Behavior on uiDepth {
                NumberAnimation {
                    duration: 1500
                    easing.type: Easing.InOutQuad
                }
            }

            property real time: 0
            NumberAnimation on time {
                loops: Animation.Infinite
                from: 0; to: 10000; duration: 10000000
                running: true
            }

            fragmentShader: "qrc:/shaders/background.frag.qsb"
        }

        // 4. (Optional) Put your Red Border back on top if you still want it
        Rectangle {
            anchors.fill: parent
            color: "transparent"
            border.color: "#ff4444"
            border.width: 12
            visible: Data.isLogging
            z: 100
        }
    }
}