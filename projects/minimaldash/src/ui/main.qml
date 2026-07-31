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

                anchors.verticalCenterOffset: 0
            }

            Speedometer {
                fontFamily: customFont.name
                speed: Data.speed
                maxSpeed: 60
            }
        }
    }
}