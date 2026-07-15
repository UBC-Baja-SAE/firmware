import QtQuick

Item {
    id: root

    property int rpm: 0
    property int maxRpm: 60
    property string fontFamily: ""

    implicitWidth:  440
    implicitHeight: 440

    readonly property real startDeg: 180
    readonly property real sweepDeg: 270
    readonly property real cx:       width  / 2
    readonly property real cy:       height / 2
    readonly property real r:        Math.min(width, height) * 0.43

    property real displayRpm: rpm
    Behavior on displayRpm {
        NumberAnimation { duration: 100 }
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

            for (var s = 0; s <= root.maxRpm; s += minorStep) {
                var angle = (root.startDeg + (s / root.maxRpm) * root.sweepDeg) * Math.PI / 180
                var ca = Math.cos(angle)
                var sa = Math.sin(angle)

                var isMajor = (s % 10 === 0)
                var tickLength = isMajor ? 24 : 12
                var thickness  = isMajor ? 4  : 2.5

                var outerRadius = root.r + 16

                ctx.beginPath()
                ctx.moveTo(root.cx + ca * (outerRadius - tickLength), root.cy + sa * (outerRadius - tickLength))
                ctx.lineTo(root.cx + ca * outerRadius,                root.cy + sa * outerRadius)
                ctx.strokeStyle = (s > 30) ? "#ff4444" : "#ffffff"
                ctx.lineWidth = thickness
                ctx.stroke()
            }
        }
    }

    // Labels
    Repeater {
        model: root.maxRpm / 10 + 1

        Text {
            required property int index

            property real angle:  (root.startDeg + (index / (root.maxRpm / 10)) * root.sweepDeg) * Math.PI / 180
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

            var angle = (root.startDeg + (root.displayRpm / root.maxRpm) * root.sweepDeg) * Math.PI / 180

            ctx.beginPath()
            ctx.moveTo(root.cx, root.cy)
            ctx.lineTo(root.cx + Math.cos(angle) * (root.r - 20),
                root.cy + Math.sin(angle) * (root.r - 20))
            ctx.strokeStyle = "white"
            ctx.lineWidth = 4
            ctx.stroke()
        }
    }

    onDisplayRpmChanged: needle.requestPaint()
    onMaxRpmChanged:     bg.requestPaint()
}