import QtQuick
import QtQuick.Window

Window {
    id: root
    visible: true
    title: "central_ecu"

    property real speed: guiBridge.values.speedometer ?? 0

    Text {
        id: text
        text: Math.round(root.speed) + " km/h"
        anchors.centerIn: parent
    }
}