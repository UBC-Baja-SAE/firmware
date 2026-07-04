import QtQuick
import QtQuick.Controls

Window {
    width: 1280
    height: 480
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

    Text {
        anchors.centerIn: parent
        text: "UBC Baja 234km/hr 80 50 90 "
        color: "white"

        // 2. Apply the loaded font's family name
        font.family: customFont.name

        font.pixelSize: 48
    }
}