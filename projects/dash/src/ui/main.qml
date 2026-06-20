import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    visible: true
    width: 1280
    height: 480

    // Locks window to fixed 1280x480 on Mac (most WMs hide/disable
    // the resize grip when min == max). Harmless on Linux since
    // eglfs forces fullscreen regardless of these.
    minimumWidth: 1280
    maximumWidth: 1280
    minimumHeight: 480
    maximumHeight: 480

    title: "Test"
    color: "#193664"  // shows as letterbox bars if aspect ratio doesn't match screen

    visibility: Qt.platform.os === "linux" ? Window.FullScreen : Window.Windowed

    // Fixed-resolution logical canvas — everything you design goes inside this,
    // not directly inside root. Scaled+centered to fit whatever window/screen
    // size actually exists, while always preserving exact 1280x480 aspect ratio.
    Item {
        id: content
        width: 1280
        height: 480
        anchors.centerIn: parent
        scale: Math.min(root.width / width, root.height / height)
        transformOrigin: Item.Center

        AnimatedImage {
            id: backgroundAnimation
            source: "assets/images/background.gif"
            anchors.fill: parent
            playing: true
            fillMode: Image.PreserveAspectCrop
        }

        FontLoader {
            id: customFont
            source: "assets/fonts/FOT-NewRodin Pro EB.otf"
        }

        // Put all your real UI/gauges/widgets inside `content`, not `root`
    }
}