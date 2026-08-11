import QtQuick

Item {
    id: root

    property int speed: 0
    property int maxSpeed: 60
    property string fontFamily: ""

    implicitWidth:  440
    implicitHeight: 440

    readonly property real startDeg: 180
    readonly property real sweepDeg: 270
    readonly property real cx:       width  / 2
    readonly property real cy:       height / 2
    readonly property real r:        Math.min(width, height) * 0.43

    property real displaySpeed: speed
    Behavior on displaySpeed {
        NumberAnimation { duration: 100 }
    }

    Text {
        anchors.centerIn: parent
        anchors. verticalCenterOffset: -80
        text: "km/h"
        color: "white"
        font.pixelSize: 20
        font.family:    root.fontFamily

    }

    // Ticks
    Canvas {
        id: bg
        anchors.fill: parent
        Component.onCompleted: requestPaint()

        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()

            var minorStep = 2

            for (var s = 0; s <= root.maxSpeed; s += minorStep) {
                var angle = (root.startDeg + (s / root.maxSpeed) * root.sweepDeg) * Math.PI / 180
                var ca = Math.cos(angle)
                var sa = Math.sin(angle)

                var isMajor = (s % 10 === 0)
                var tickLength = isMajor ? 24 : 12
                var thickness  = isMajor ? 4  : 2.5

                var outerRadius = root.r + 16

                ctx.beginPath()
                ctx.moveTo(root.cx + ca * (outerRadius - tickLength), root.cy + sa * (outerRadius - tickLength))
                ctx.lineTo(root.cx + ca * outerRadius,                root.cy + sa * outerRadius)

                ctx.strokeStyle = (s > 45) ? "#ff4444" : "#ffffff"

                ctx.lineWidth = thickness
                ctx.stroke()
            }
        }
    }

    // Labels
    Repeater {
        model: root.maxSpeed / 10 + 1

        Text {
            required property int index

            property real angle:  (root.startDeg + (index / (root.maxSpeed / 10)) * root.sweepDeg) * Math.PI / 180
            property real labelR: root.r - 38

            text:           String(index * 10)
            font.family:    root.fontFamily
            font.pixelSize: 24
            color:          "white"

            x: root.cx + Math.cos(angle) * labelR - width  / 2
            y: root.cy + Math.sin(angle) * labelR - height / 2
        }
    }

    // Needle
    Canvas {
        id: needle
        anchors.fill: parent

        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()

            var angle = (root.startDeg + (root.displaySpeed / root.maxSpeed) * root.sweepDeg) * Math.PI / 180

            ctx.beginPath()
            ctx.moveTo(root.cx, root.cy)
            ctx.lineTo(root.cx + Math.cos(angle) * (root.r - 20),
                root.cy + Math.sin(angle) * (root.r - 20))
            ctx.strokeStyle = "white"
            ctx.lineWidth = 4
            ctx.stroke()
        }
    }

    onDisplaySpeedChanged: needle.requestPaint()
    onMaxSpeedChanged:     bg.requestPaint()
}