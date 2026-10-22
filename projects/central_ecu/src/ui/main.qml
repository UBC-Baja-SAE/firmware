import QtQuick
import QtQuick.Window

Window {
    visible: true
    title: "central_ecu"

    Image {
        id: myImage
        source: "qrc:/images/ryan.jpeg"

        anchors.centerIn: parent
        fillMode: Image.PreserveAspectFit
        width: 400
        height: 400
    }
}