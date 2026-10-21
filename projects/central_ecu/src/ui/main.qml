import QtQuick
import QtQuick.Window

Window {
    width: 200
    height: 200
    visible: true
    title: "central_ecu"

    Image {
        id: myImage
        source: "qrc:/images/ryan.jpeg"

        anchors.centerIn: parent
        fillMode: Image.PreserveAspectFit
        width: 200
        height: 200
    }

}