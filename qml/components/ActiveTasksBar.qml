import QtQuick
import QtQuick.Layouts
import Polomodoro

// The only place the main window shows tasks. Pause/Stop are ALWAYS visible
// here (unlike TaskRow): the strip exists to make those two reachable.
Rectangle {
    id: root
    implicitHeight: flow.implicitHeight + Theme.md * 2
    color: Qt.rgba(0.078, 0.086, 0.106, Theme.panelOpacity)
    border.color: Theme.panelBorder
    border.width: 1
    radius: Theme.rPanel

    Flow {
        id: flow
        anchors.fill: parent
        anchors.margins: Theme.md
        spacing: Theme.sm

        Text {
            text: "ACTIVE"
            font.family: Theme.monoFamily
            font.pixelSize: 11
            font.letterSpacing: 1.8
            color: Qt.alpha(Theme.textPrimary, 0.5)
            height: 34
            verticalAlignment: Text.AlignVCenter
            leftPadding: Theme.sm - 2
            rightPadding: Theme.sm - 2
        }

        Repeater {
            // Caps at 4; the remainder collapses into a +n chip.
            model: TaskController.activeChips
            Rectangle {
                height: 34
                width: chip.implicitWidth + Theme.md * 2
                radius: Theme.rRow
                color: Theme.surfaceRaised
                border.width: 1
                border.color: Theme.accent
                Behavior on border.color { ColorAnimation { duration: Theme.dAccent } }

                RowLayout {
                    id: chip
                    anchors.centerIn: parent
                    spacing: Theme.sm + 1
                    Rectangle { width: 6; height: 6; radius: 3; color: Theme.accent }
                    Text { text: modelData.title; font.family: Theme.fontFamily; font.pixelSize: Theme.fBodyS; color: Theme.textPrimary }
                    Text {
                        text: modelData.liveLabel
                        font.family: Theme.monoFamily; font.pixelSize: Theme.fNumeric
                        color: modelData.overTarget ? Theme.overflow : Theme.accentHover
                    }
                    IconButton { glyph: "pause"; small: true; onClicked: TaskController.pauseTask(modelData.id) }
                    IconButton { glyph: "stop";  small: true; onClicked: TaskController.stopTask(modelData.id) }
                }
            }
        }

        PillButton {
            visible: TaskController.activeOverflowCount > 0
            label: "+" + TaskController.activeOverflowCount + " more"
            onClicked: TaskController.openDrawerOnActive()
        }
    }
}
