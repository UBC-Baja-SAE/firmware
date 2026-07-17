import QtQuick
import QtMultimedia

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

    VideoOutput {
        id: videoOutput
        anchors.fill: parent
        anchors.margins: camRect.border.width // Avoid overlapping the white border
        fillMode: VideoOutput.PreserveAspectCrop // Fills the container perfectly
    }

    // 3. Route the backend stream directly to the video element
    Component.onCompleted: {
        if (IsReleaseBuild && typeof WebcamBackend !== "undefined") {
            WebcamBackend.setQmlVideoOutput(videoOutput)
        }
    }



}