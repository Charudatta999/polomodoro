import QtQuick
import QtQuick.Layouts
import Polomodoro
import "../components"

// PiP — 300x140 default, frameless, draggable by its background.
Rectangle {
    id: root
    color: Qt.rgba(0.055, 0.059, 0.075, 0.94)
    border.color: Qt.rgba(0.929, 0.937, 0.953, 0.12)
    border.width: 1
    radius: Theme.rPanel

    // Exactly one active task with a target → show its bar. Two or more and a
    // single bar at this size would misrepresent an aggregate.
    readonly property var soleTarget: TaskController.soleTargetedActiveTask

    DragHandler {
        target: null
        onActiveChanged: if (active) Window.window.startSystemMove()
    }
    TapHandler {
        onDoubleTapped: WindowLayoutManager.setMode(0)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.md + 2
        spacing: Theme.sm

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.sm

            Text {
                text: TimerController.formattedTime
                font.family: Theme.monoFamily
                font.pixelSize: 34
                font.weight: Font.Medium
                font.features: { "tnum": 1 }
                color: Theme.textPrimary
            }
            Item { Layout.fillWidth: true }
            IconButton { glyph: TimerController.isRunning ? "pause" : "play"; filled: true; onClicked: TimerController.toggle() }
            IconButton { glyph: "stop"; onClicked: TimerController.reset() }
            IconButton { glyph: "expand"; onClicked: WindowLayoutManager.setMode(0) }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.xs + 2

            RowLayout {
                Layout.fillWidth: true
                visible: !!root.soleTarget
                Text {
                    text: root.soleTarget ? root.soleTarget.title : ""
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fNumeric
                    color: Theme.textDim
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
                Text {
                    text: root.soleTarget ? root.soleTarget.progressLabel : ""
                    font.family: Theme.monoFamily
                    font.pixelSize: 11
                    color: Theme.textDim
                }
            }

            ProgressTrough {
                Layout.fillWidth: true
                visible: !!root.soleTarget
                ratio: root.soleTarget ? root.soleTarget.progressRatio : 0
            }

            Text {
                text: TaskController.activeCount + " tasks active · " + TimerController.phaseLabel
                font.family: Theme.monoFamily
                font.pixelSize: Theme.fEyebrow
                color: Theme.textFaint
            }
        }
    }
}
