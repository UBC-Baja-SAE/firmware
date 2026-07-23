import QtQuick
import QtMultimedia

Item {
    id: root

    implicitWidth:  640
    implicitHeight: 240

    Rectangle {
        id: camRect
        anchors.centerIn: parent
        width: parent.width - 70
        height: parent.height - 70
        color: "black"
    }

    VideoOutput {
        id: videoOutput
        anchors.fill: camRect
        anchors.margins: camRect.border.width
        fillMode: VideoOutput.PreserveAspectCrop
    }

    Component.onCompleted: {
        if (IsReleaseBuild && typeof WebcamBackend !== "undefined") {
            WebcamBackend.setQmlVideoOutput(videoOutput)
        }
    }
}