import QtQuick

Item {
    id: root
    implicitWidth: 400
    implicitHeight: 240

    // A dictionary to route incoming messages to the correct dot efficiently
    property var dotMap: ({})

    // Call this function when a new frame is received from your DbcParser
    function processCanMessage(topicName, payload) {
        if (dotMap[topicName]) {
            dotMap[topicName].updateStatus(payload)
        }
    }

    /*
     * HOW TO CONNECT C++ TO THIS COMPONENT:
     * Assuming you expose your DbcParser to QML (e.g., as a context property named `dbcParser`),
     * uncomment the Connections block below to automatically route the signals.
     */
    Connections {
        target: Data

        function onFrameForwardedToQml(topicName, payload) {
            root.processCanMessage(topicName, payload)
        }
    }

    // Reusable Indicator Dot Component
    component CanDot : Rectangle {
        id: dot
        width: 16
        height: 16
        radius: width / 2

        property string topic: ""
        property int status: 0 // 0: Red, 1: Yellow, 2: Green

        color: status === 0 ? "#FF3B30" : (status === 1 ? "#FFCC00" : "#34C759")
        border.color: "#1C1C1E"
        border.width: 1

        property string _lastPayloadStr: ""

        // 1. Timer for complete data loss (Red)
        // If no messages arrive at all for 1.5s, it goes Red.
        Timer {
            id: redTimer
            interval: 1500
            repeat: false
            onTriggered: {
                dot.status = 0
                yellowTimer.stop()
            }
        }

        // 2. Timer for unchanging data (Yellow)
        // If messages arrive but haven't changed for 3s, it drops to Yellow.
        Timer {
            id: yellowTimer
            interval: 3000
            repeat: false
            onTriggered: {
                if (dot.status === 2) {
                    dot.status = 1
                }
            }
        }

        function updateStatus(payload) {
            let cleanPayload = Object.assign({}, payload)
            delete cleanPayload.timestamp
            let currentString = JSON.stringify(cleanPayload)

            // A message arrived! Prevent the dot from going Red.
            redTimer.restart()

            if (currentString !== _lastPayloadStr) {
                // The data is actively changing: Go Green
                dot.status = 2
                _lastPayloadStr = currentString

                // Restart the countdown to Yellow
                yellowTimer.restart()
            } else if (dot.status === 0) {
                // Edge case: Node just came back online (from Red),
                // but the values are identical to before it died.
                dot.status = 1
            }
        }

        Component.onCompleted: {
            if (topic !== "") root.dotMap[topic] = this
        }
        Component.onDestruction: {
            if (topic !== "") delete root.dotMap[topic]
        }
    }

    // Layout grouping: 9, 4, and 1
    Row {
        anchors.centerIn: parent
        spacing: 30

        // Group of 9 (3x3 grid)
        Grid {
            columns: 3
            spacing: 10
            anchors.verticalCenter: parent.verticalCenter

            CanDot { topic: "fr_accel" }
            CanDot { topic: "fr_gyro" }
            CanDot { topic: "fr_linpot" }

            CanDot { topic: "fl_accel" }
            CanDot { topic: "fl_gyro" }
            CanDot { topic: "fl_linpot" }

            CanDot { topic: "rr_accel" }
            CanDot { topic: "rr_gyro" }
            CanDot { topic: "rr_linpot" }
        }

        // Group of 4 (2x2 grid)
        Grid {
            columns: 2
            spacing: 10
            anchors.verticalCenter: parent.verticalCenter

            CanDot { topic: "rear_accel" }
            CanDot { topic: "rear_gyro" }
            CanDot { topic: "speedometer" }
            CanDot { topic: "tachometer" }
        }

        // Group of 1
        Item {
            width: 16
            height: 16
            anchors.verticalCenter: parent.verticalCenter

            CanDot {
                anchors.centerIn: parent
                topic: "front_steering"
            }
        }
    }
}