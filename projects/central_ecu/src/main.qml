import QtQuick
import QtQuick.Window
import QtQuick.Layouts

Window {
    id: root
    width: 400
    height: 240
    minimumWidth: 400
    minimumHeight: 240
    visible: true
    title: "Roxanne Dashboard"
    color: AppState.bg

    readonly property var labels: ["Time Elapsed", "Temperature", "Battery Level", "Connectivity"]
    readonly property var values: [AppState.formatTime(AppState.elapsedSeconds),
        AppState.tempText, AppState.battText, AppState.ipText]

    Shortcut { sequence: "Space"; onActivated: AppState.dark = !AppState.dark }
    Shortcut { sequence: "R"; onActivated: AppState.recording = !AppState.recording }   // recording test key
    Shortcut { sequence: "Escape"
        onActivated: {
            console.log("Shutting down")
            Qt.quit()
        }
    }

    component StatTile: Item {
        property string label
        property string value
        property color textColor: "black"
        property bool divider: false

        Rectangle {
            visible: divider
            width: 1
            height: parent.height - 16
            anchors.verticalCenter: parent.verticalCenter
            color: textColor
        }
        Column {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: divider ? 7 : 0
            Text { width: parent.width; text: label; color: textColor; font.pixelSize: 10; elide: Text.ElideRight }
            Text { width: parent.width; text: value; color: textColor; font.pixelSize: 16; elide: Text.ElideRight }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Item {
            id: main
            Layout.fillWidth: true
            Layout.fillHeight: true

            readonly property real inner: width - 3 * AppState.margin

            GPS {
                x: AppState.margin
                y: AppState.margin
                width: main.inner * 2 / 3
                height: parent.height - 2 * AppState.margin
            }

            Data {
                x: 2 * AppState.margin + main.inner * 2 / 3
                y: AppState.margin
                width: main.inner / 3
                height: parent.height - 2 * AppState.margin
            }
        }

        // bottom bar
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            color: AppState.bg

            Rectangle { width: parent.width; height: 1; color: AppState.fg }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: AppState.margin
                anchors.rightMargin: AppState.margin
                spacing: 6

                Repeater {
                    model: root.labels.length
                    StatTile {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.preferredWidth: 1
                        label: root.labels[index]
                        value: root.values[index]
                        textColor: AppState.fg
                        divider: index > 0
                    }
                }
            }
        }
    }
}