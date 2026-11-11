pragma Singleton
import QtQuick

QtObject {
    // theme stuff
    property bool dark: false
    readonly property color bg: dark ? "black" : "white"
    readonly property color fg: dark ? "white" : "black"
    readonly property int margin: 8

    property FontLoader mono: FontLoader { source: "qrc:/fonts/commitmono.otf" }
    readonly property string fontFamily: mono.name

    // temp values
    property bool recording: false
    property int elapsedSeconds: 0
    property string tempText: "0 °C"
    property string battText: "12.2V"
    property string ipText: "127.0.0.1"

    // temp gps
    property var path: []
    property int pathTick: 0
    property real phase: 0

    function pad(n) { return n < 10 ? "0" + n : "" + n }
    function formatTime(t) {
        return pad(Math.floor(t / 3600)) + ":" + pad(Math.floor(t % 3600 / 60)) + ":" + pad(t % 60)
    }

    property Timer elapsedTimer: Timer {
        interval: 1000
        running: recording
        repeat: true
        onTriggered: elapsedSeconds++
    }

    // fake gps
    property Timer gpsTimer: Timer {
        interval: 100
        running: true
        repeat: true
        onTriggered: {
            phase += 0.03
            var a = phase
            var p = { x: 120 * Math.cos(a) + 30 * Math.cos(3 * a),
                y: 70 * Math.sin(a) + 20 * Math.sin(2 * a) }
            path = path.concat([p]).slice(-120)
            pathTick++
        }
    }
}