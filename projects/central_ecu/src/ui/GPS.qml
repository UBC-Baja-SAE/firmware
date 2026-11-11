import QtQuick

Panel {
    title: "GPS"

    Canvas {
        id: gpsCanvas
        anchors.fill: parent
        anchors.margins: 1

        property var path: AppState.path
        property int tick: AppState.pathTick
        property color ink: AppState.fg
        property real pxPerMetre: 4

        onTickChanged: requestPaint()
        onInkChanged: requestPaint()
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()

        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)

            var s = pxPerMetre
            var n = path.length
            var cx = width / 2, cy = height / 2
            var px = n ? path[n - 1].x : 0
            var py = n ? path[n - 1].y : 0

            ctx.strokeStyle = String(ink)
            ctx.fillStyle = String(ink)
            ctx.lineJoin = "round"
            ctx.lineCap = "round"

            if (n > 1) {
                ctx.lineWidth = 2
                for (var k = 1; k < n; k++) {
                    ctx.globalAlpha = 0.15 + 0.85 * k / n
                    ctx.beginPath()
                    ctx.moveTo(cx + (path[k - 1].x - px) * s, cy - (path[k - 1].y - py) * s)
                    ctx.lineTo(cx + (path[k].x - px) * s, cy - (path[k].y - py) * s)
                    ctx.stroke()
                }
            }

            ctx.globalAlpha = 1
            ctx.fillRect(cx - 4, cy - 4, 8, 8)
        }
    }
}