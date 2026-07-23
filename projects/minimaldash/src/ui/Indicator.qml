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
        target: DbcParser
        function onFrameParsed(topicName, payload) {
            root.processCanMessage(topicName, payload)
        }
    }

    // Reusable Indicator Dot Component
    component CanDot : Rectangle {
        id: dot
        width: 16
        height: 16
        radius: width / 2

        // Map this to the specific CAN message name (from your DBC file)
        property string topic: ""

        // 0: Red (No data), 1: Yellow (Unchanged), 2: Green (Changing)
        property int status: 0

        color: status === 0 ? "#FF3B30" : (status === 1 ? "#FFCC00" : "#34C759")
        border.color: "#1C1C1E"
        border.width: 1

        property string _lastPayloadStr: ""

        // Timer resets on message; if it fires, we assume the message stopped coming
        Timer {
            id: timeoutTimer
            interval: 1500 // 1.5 seconds without a message triggers Red
            repeat: false
            onTriggered: dot.status = 0
        }

        function updateStatus(payload) {
            // Create a shallow copy and remove the timestamp for an accurate data comparison
            let cleanPayload = Object.assign({}, payload)
            delete cleanPayload.timestamp

            // Convert to string to easily check if the values have actually changed
            let currentString = JSON.stringify(cleanPayload)

            if (currentString === _lastPayloadStr) {
                dot.status = 1 // Yellow (Data is coming, but values aren't changing)
            } else {
                dot.status = 2 // Green (Data is coming and values are updating)
                _lastPayloadStr = currentString
            }

            timeoutTimer.restart()
        }

        // Register and unregister from the parent map dynamically
        Component.onCompleted: {
            if (topic !== "") {
                root.dotMap[topic] = this
            }
        }
        Component.onDestruction: {
            if (topic !== "") {
                delete root.dotMap[topic]
            }
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