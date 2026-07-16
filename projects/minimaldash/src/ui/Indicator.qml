import QtQuick

Item {
    id: root

    implicitWidth:  240
    implicitHeight: 240

    Rectangle {
        anchors.centerIn: parent
        width: parent.width - 40
        height: parent.height -40
        color: "blue"
        border.width: 3
        border.color: "white"
    }



}