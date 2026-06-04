import QtQuick 2.15

Item {
    id: root
    // Default size, can be overridden by the main layout
    width: 480
    height: 480

    property real value: 0
    property real minValue: 0
    property real maxValue: 120
    property string unitText: "KM/H"
    property int textUpdateInterval: 50
    property bool noSignal: false

    // --- DYNAMIC PROPERTIES ---
    property var rainbowColors: ["#FF0000", "#ffc400", "#FFFF00", "#33FF00", "#00f7ff", "#9c33ff"]
    property string gifSource: "images/original.gif"
    property real gifWidth: 120
    property real gifHeight: 74
    // --------------------------

    property int trackWidth: 30
    property color trackColor: "#03101e"

    property real _animatedValue: value
    property string _displayedText: "0"

    property real _tipX: width / 2
    property real _tipY: height / 2
    property real _tipRotation: 0

    Behavior on _animatedValue {
        NumberAnimation { 
            // Match this duration roughly to your ROS 2 publish rate.
            // 50ms to 100ms creates a buttery smooth, mechanical tick.
            duration: 75 
            easing.type: Easing.OutSine 
        }
    }

    FontLoader {
        id: newRodinFont
        // Ensure you create a 'fonts' folder in your gui directory and put the .otf file there
        source: "fonts/FOT-NewRodin_Pro_EB.otf" 
    }

    Timer {
        id: textThrottle
        interval: root.textUpdateInterval
        running: true
        repeat: true
        onTriggered: {
            root._displayedText = Math.round(root._animatedValue).toString()
        }
    }

    Canvas {
        id: canvas
        anchors.fill: parent
        opacity: root.noSignal ? 0.25 : 1.0
        Behavior on opacity { NumberAnimation { duration: 300 } }

        Connections {
            target: root
            function on_AnimatedValueChanged() { canvas.requestPaint(); }
        }

        onPaint: {
            var ctx = getContext("2d");
            ctx.clearRect(0, 0, width, height);

            var centerX = width / 2;
            var centerY = height / 2;
            var baseRadius = (width / 2) - root.trackWidth - 30;
            var startAngle = Math.PI * 0.75;
            var endAngle = Math.PI * 2.25;

            // Background Track
            ctx.globalAlpha = 0.2;
            ctx.beginPath();
            ctx.arc(centerX, centerY, baseRadius, startAngle, endAngle);
            ctx.lineWidth = root.trackWidth;
            ctx.strokeStyle = root.trackColor;
            ctx.lineCap = "butt";
            ctx.stroke();
            ctx.globalAlpha = 1.0;

            var range = root.maxValue - root.minValue;
            var clampedValue = Math.max(root.minValue, Math.min(root._animatedValue, root.maxValue));
            var fillPercentage = (clampedValue - root.minValue) / range;
            var valueAngle = startAngle + (fillPercentage * (endAngle - startAngle));

            var stripeWidth = root.trackWidth / root.rainbowColors.length;

            for (var i = 0; i < root.rainbowColors.length; i++) {
                var offset = (i * stripeWidth) - (root.trackWidth / 2) + (stripeWidth / 2);
                var stripeRadius = baseRadius + offset;

                ctx.beginPath();
                ctx.arc(centerX, centerY, stripeRadius, startAngle, valueAngle);
                ctx.lineWidth = stripeWidth;
                ctx.strokeStyle = root.rainbowColors[i];
                ctx.lineCap = "butt";
                ctx.stroke();
            }

            root._tipX = centerX + baseRadius * Math.cos(valueAngle);
            root._tipY = centerY + baseRadius * Math.sin(valueAngle);
            root._tipRotation = (valueAngle + (Math.PI / 2)) * (180 / Math.PI);
        }
    }

    AnimatedImage {
        id: nyanCatCap
        source: root.gifSource
        width: root.gifWidth
        height: root.gifHeight

        fillMode: Image.PreserveAspectFit
        x: root._tipX - (width / 2)
        y: root._tipY - (height / 2)
        rotation: root._tipRotation
        transformOrigin: Item.Center

        opacity: (root._animatedValue > root.minValue && !root.noSignal) ? 1.0 : 0.0
        Behavior on opacity { NumberAnimation { duration: 200 } }
    }

    Text {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -20
        text: root.noSignal ? "N/A" : root._displayedText
        color: root.noSignal ? "#ff4444" : "#FFFFFF"
        font.pixelSize: root.noSignal ? 72 : 108
        font.family: newRodinFont.name
        font.bold: true
    }

    Text {
        anchors.top: parent.verticalCenter
        anchors.topMargin: 36
        anchors.horizontalCenter: parent.horizontalCenter
        text: root.unitText
        color: root.noSignal ? "#ff4444" : "#FFFFFF"
        font.pixelSize: 24
        font.family: newRodinFont.name
        font.bold: true
        font.letterSpacing: 2
    }
}
