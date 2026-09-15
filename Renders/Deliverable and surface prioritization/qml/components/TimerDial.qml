import QtQuick
import QtQuick.Shapes
import Polomodoro

// Ring arc + centred digits. Shape (not Canvas) — a Canvas repaint at 10Hz
// shows up in QSG_RENDER_TIMING; Shape lives on the scene graph and costs
// nothing per tick.
Item {
    id: root
    property int diameter: 280
    readonly property color ringColor: Theme.phaseColor(TimerController.phase)
    readonly property real stroke: Math.max(6, diameter * 0.046)

    implicitWidth: diameter
    implicitHeight: diameter

    Shape {
        anchors.fill: parent
        antialiasing: true
        layer.enabled: true
        layer.samples: 4

        ShapePath {
            strokeColor: Theme.surfaceRaised
            strokeWidth: root.stroke
            fillColor: "transparent"
            capStyle: ShapePath.FlatCap
            PathAngleArc {
                centerX: root.diameter / 2; centerY: root.diameter / 2
                radiusX: (root.diameter - root.stroke) / 2
                radiusY: (root.diameter - root.stroke) / 2
                startAngle: -90; sweepAngle: 360
            }
        }

        ShapePath {
            strokeColor: root.ringColor
            strokeWidth: root.stroke
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap
            PathAngleArc {
                id: arc
                centerX: root.diameter / 2; centerY: root.diameter / 2
                radiusX: (root.diameter - root.stroke) / 2
                radiusY: (root.diameter - root.stroke) / 2
                startAngle: -90
                // Single qreal on the controller, emitted with remainingMs so
                // the ring never lags the digits by a binding pass.
                sweepAngle: 360 * TimerController.progress
                // Linear, 100ms: matches the tick source exactly. Easing here
                // makes the arc visibly trail the numbers.
                Behavior on sweepAngle { NumberAnimation { duration: 100 } }
            }
            Behavior on strokeColor { ColorAnimation { duration: Theme.dAccent; easing.type: Easing.InOutQuad } }
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: Theme.sm - 1

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: TimerController.phaseLabel.toUpperCase()
            font.family: Theme.monoFamily
            font.pixelSize: Math.max(10, root.diameter * 0.042)
            font.letterSpacing: 2
            color: root.ringColor
            Behavior on color { ColorAnimation { duration: Theme.dAccent } }
        }

        // Three items so the tick animation can target the seconds only.
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            Text {
                text: TimerController.minutes
                font.family: Theme.monoFamily
                font.pixelSize: root.diameter * 0.25
                font.weight: Font.Medium
                font.features: { "tnum": 1 }
                color: TimerController.isPaused ? Qt.alpha(Theme.textPrimary, 0.62) : Theme.textPrimary
            }
            Text {
                text: ":"
                width: root.diameter * 0.075   // fixed, or the group shifts
                horizontalAlignment: Text.AlignHCenter
                font.family: Theme.monoFamily
                font.pixelSize: root.diameter * 0.25
                color: Theme.textPrimary
            }
            Text {
                id: secs
                text: TimerController.seconds
                font.family: Theme.monoFamily
                font.pixelSize: root.diameter * 0.25
                font.weight: Font.Medium
                font.features: { "tnum": 1 }
                color: TimerController.isPaused ? Qt.alpha(Theme.textPrimary, 0.62) : Theme.textPrimary
                onTextChanged: tick.restart()
                SequentialAnimation {
                    id: tick
                    NumberAnimation { target: secs; property: "opacity"; to: 0.72; duration: 70 }
                    NumberAnimation { target: secs; property: "opacity"; to: 1.0;  duration: 70 }
                }
            }
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 5
            visible: TimerController.mode === "pomodoro"
            Repeater {
                model: SettingsController.cyclesBeforeLongBreak
                Rectangle {
                    width: 7; height: 7; radius: 3.5
                    color: index < TimerController.cycleIndex ? root.ringColor : Theme.line
                    Behavior on color { ColorAnimation { duration: Theme.dAccent } }
                }
            }
        }
    }

    TapHandler { onTapped: TimerController.toggle() }
}
