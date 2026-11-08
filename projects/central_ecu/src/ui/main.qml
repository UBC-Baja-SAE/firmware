import QtQuick

Window {
    width: 400
    height: 240
    visible: true

    FontLoader {
        id: suse
        source: "qrc:/fonts/CommitMono-Regular.otf"
    }

    Item {
        anchors.fill: parent
        focus: true

        Keys.onEscapePressed: {
            console.log("Shutting down")
            Qt.quit()
        }

        Text {
            anchors.centerIn: parent
            text: "UBC Baja"
            color: "black"
            font.pixelSize: 48
            font.family: suse.name
        }
    }
}