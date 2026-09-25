import QtQuick
import QtQuick.Layouts
import Polomodoro
import "../components"

Rectangle {
    id: root
    color: Qt.rgba(0.055, 0.059, 0.075, 0.94)
    border.color: Theme.panelBorder
    border.width: 1
    radius: Theme.rPanel

    readonly property var soleTarget: TaskController.soleTargetedActiveTask

    // Background drag. A MouseArea declared first sits below every control, so
    // the transport, progress bar and buttons still get their own events and
    // only bare background starts a move — "draggable by their background".
    //
    // A MouseArea rather than a DragHandler: Wayland validates startSystemMove
    // against the input serial of the event that triggered it, and a handler
    // that only activates after the drag threshold presents a stale serial,
    // which the compositor quietly refuses.
    MouseArea {
        id: backgroundDrag
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        // On first motion rather than on press, so a plain click still lands
        // (PiP double-click expands) while the serial is still current.
        onPressed: if (Window.window && Window.window.resetSystemMove) Window.window.resetSystemMove()
        onPositionChanged: if (pressed && Window.window && Window.window.beginSystemMove) Window.window.beginSystemMove()
        onDoubleClicked: WindowLayoutManager.setMode(0)
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
