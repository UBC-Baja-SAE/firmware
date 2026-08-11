import QtQuick

Item {
    id: root

    property real angle: 0
    property real maxAngle: 30
    property string fontFamily: ""

    implicitWidth:  240
    implicitHeight: 240

    readonly property real startDeg: 240
    readonly property real sweepDeg: 60
    readonly property real cx:       width  / 2
    readonly property real cy:       height / 2
    readonly property real r:        Math.min(width, height) * 0.40

    property real displayAngle: angle
    Behavior on displayAngle {
        NumberAnimation { duration: 100 }
    }

    Text {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -50
        text: "deg"
        color: "white"
        font.pixelSize: 14
        font.family: root.fontFamily
    }

    // Ticks
    Canvas {
        id: bg
        anchors.fill: parent
        Component.onCompleted: requestPaint()

        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()

            var step = 10

            for (var s = -root.maxAngle; s <= root.maxAngle; s += step) {
                var normalized = (s + root.maxAngle) / (2 * root.maxAngle)
                var rad = (root.startDeg + normalized * root.sweepDeg) * Math.PI / 180
                var ca = Math.cos(rad)
                var sa = Math.sin(rad)

                var isMajor = (s === 0 || Math.abs(s) === root.maxAngle)
                var tickLength = isMajor ? 16 : 10
                var thickness  = isMajor ? 3  : 2

                var outerRadius = root.r + 10

                ctx.beginPath()
                ctx.moveTo(root.cx + ca * (outerRadius - tickLength), root.cy + sa * (outerRadius - tickLength))
                ctx.lineTo(root.cx + ca * outerRadius,                root.cy + sa * outerRadius)

                ctx.strokeStyle = "#ffffff"
                ctx.lineWidth = thickness
                ctx.stroke()
            }
        }
    }

    // Labels
    Repeater {
        model: [-30, 0, 30]

        Text {
            required property int modelData

            property real normalized: (modelData + root.maxAngle) / (2 * root.maxAngle)
            property real rad:        (root.startDeg + normalized * root.sweepDeg) * Math.PI / 180
            property real labelR:     root.r - 24 // Adjusted padding for larger text

            text:           String(modelData)
            font.family:    root.fontFamily
            font.pixelSize: 18
            color:          "white"

            x: root.cx + Math.cos(rad) * labelR - width  / 2
            y: root.cy + Math.sin(rad) * labelR - height / 2
        }
    }

    // Needle
    Canvas {
        id: needle
        anchors.fill: parent

        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()

            var clampedVal = Math.max(-root.maxAngle, Math.min(root.maxAngle, root.displayAngle))
            var normalized = (clampedVal + root.maxAngle) / (2 * root.maxAngle)
            var rad = (root.startDeg + normalized * root.sweepDeg) * Math.PI / 180

            ctx.beginPath()
            ctx.moveTo(root.cx, root.cy)
            ctx.lineTo(root.cx + Math.cos(rad) * (root.r - 12),
                root.cy + Math.sin(rad) * (root.r - 12))
            ctx.strokeStyle = "white"
            ctx.lineWidth = 3
            ctx.stroke()
        }
    }

    onDisplayAngleChanged: needle.requestPaint()
    onMaxAngleChanged:     bg.requestPaint()
}