import QtQuick
import QtQuick.Controls

Window {
    width: 640
    height: 480
    visible: true

    Column {
        anchors.centerIn: parent
        spacing: 20

        Row {
            spacing: 15
            Label {
                text: "WebSocket"
                anchors.verticalCenter: parent.verticalCenter
            }
            Switch {
                checked: AppSettings.websocketEnabled
                onCheckedChanged: AppSettings.websocketEnabled = checked
            }
        }

        Row {
            spacing: 15
            Label {
                text: "Mcap Logging"
                anchors.verticalCenter: parent.verticalCenter
            }
            Switch {
                checked: AppSettings.mcapEnabled
                onCheckedChanged: AppSettings.mcapEnabled = checked
            }
        }
    }
}