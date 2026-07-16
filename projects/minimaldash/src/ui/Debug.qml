import QtQuick

Item {
    id: root

    implicitWidth:  640
    implicitHeight: 240

    Rectangle {
        anchors.centerIn: parent
        width: parent.width - 70
        height: parent.height - 70
        color: "blue"
        border.width: 3
        border.color: "white"
    }



}