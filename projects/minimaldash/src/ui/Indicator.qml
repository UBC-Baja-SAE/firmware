import QtQuick

Item {
    id: root

    implicitWidth:  240
    implicitHeight: 240

    Item {
        anchors.centerIn: parent
        width: parent.width - 20
        height: parent.height - 20

        // Add the Image component here
        Image {
            anchors.centerIn: parent
            source: "qrc:/qt/qml/app/assets/icons/logo.png" // Replace with the actual path to your PNG

            // Optional: If your PNG is larger than the rectangle, uncomment these to scale it
            width: parent.width
            height: parent.height
            fillMode: Image.PreserveAspectFit
        }
    }
}