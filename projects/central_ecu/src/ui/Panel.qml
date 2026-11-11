import QtQuick

Rectangle {
    property string title
    color: AppState.bg
    border.color: AppState.fg
    border.width: 1

    Rectangle {
        z: 1
        x: 1
        y: 1
        width: label.x + label.implicitWidth + AppState.margin
        height: label.y + label.implicitHeight + AppState.margin
        color: AppState.bg

        Text {
            id: label
            x: AppState.margin - 1
            y: AppState.margin - 1
            text: title
            color: AppState.fg
            font.family: AppState.fontFamily
            font.pixelSize: 16
        }
    }
}