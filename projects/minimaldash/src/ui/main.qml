import QtQuick
import QtQuick.Controls

Window {
    id: root

    width: Qt.platform.os === "linux" ? 480 : 1280
    height: Qt.platform.os === "linux" ? 1280 : 480
    visible: true
    color: "black"

    minimumWidth: width
    maximumWidth: width
    minimumHeight: height
    maximumHeight: height

    FontLoader {
        id: customFont
        source: "qrc:/qt/qml/app/assets/fonts/microgramma.ttf"
    }

    Item {
        id: dashContainer

        width: 1280
        height: 480
        anchors.centerIn: parent

        rotation: Qt.platform.os === "linux" ? 90 : 0

        Text {
            anchors.centerIn: parent
            text: "UBC Baja 234km/hr 80 50 90 "
            color: "white"
            font.family: customFont.name
            font.pixelSize: 48
        }
    }
}